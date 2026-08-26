/* Original UDM WFP monitor. Explicit PID scope, connection metadata and TCP/UDP byte
 * counts. Inspection continues classification; no content copy or reinjection. */
#pragma warning(push)
#pragma warning(disable:4201 4324)
#include <ntddk.h>
#include <initguid.h>
#include <fwpsk.h>
#include <fwpmk.h>
#include <wdmsec.h>
#pragma warning(pop)
#include "UdmWfpProtocol.h"
DEFINE_GUID(UDM_SUBLAYER,0xb21d9c81,0x4f26,0x4c44,0xa7,0xf4,0x61,0x09,0xf0,0x51,0xa8,0x22);
DEFINE_GUID(UDM_FLOW4,0xb21d9c84,0x4f26,0x4c44,0xa7,0xf4,0x61,0x09,0xf0,0x51,0xa8,0x22);
DEFINE_GUID(UDM_FLOW6,0xb21d9c85,0x4f26,0x4c44,0xa7,0xf4,0x61,0x09,0xf0,0x51,0xa8,0x22);
DEFINE_GUID(UDM_STREAM4,0xb21d9c86,0x4f26,0x4c44,0xa7,0xf4,0x61,0x09,0xf0,0x51,0xa8,0x22);
DEFINE_GUID(UDM_STREAM6,0xb21d9c87,0x4f26,0x4c44,0xa7,0xf4,0x61,0x09,0xf0,0x51,0xa8,0x22);
DEFINE_GUID(UDM_DATAGRAM4,0xb21d9c88,0x4f26,0x4c44,0xa7,0xf4,0x61,0x09,0xf0,0x51,0xa8,0x22);
DEFINE_GUID(UDM_DATAGRAM6,0xb21d9c89,0x4f26,0x4c44,0xa7,0xf4,0x61,0x09,0xf0,0x51,0xa8,0x22);
DEFINE_GUID(UDM_DEVICE_CLASS,0x981efb77,0x491c,0x42b4,0xb2,0x45,0xd4,0x63,0x36,0x70,0xa9,0x38);
static PDEVICE_OBJECT gDevice;static HANDLE gEngine;static UINT32 gCallout[6];
static KSPIN_LOCK gLock;static EX_RUNDOWN_REF gRundown;static KEVENT gNoFlows;
static ULONG gFlowRefs,gWatchCount;static UINT32 gWatch[UDM_WFP_MAX_PIDS];
static UDM_WFP_FLOW gFlows[UDM_WFP_MAX_FLOWS];
static UINT64 gGeneration,gDropped,gConnections,gReceived,gSent;
static volatile LONG gStopping;static BOOLEAN gLinkCreated;
static DRIVER_DISPATCH Dispatch;
static DRIVER_UNLOAD Unload;
static UINT64 Now(void){LARGE_INTEGER time;KeQuerySystemTime(&time);return (UINT64)time.QuadPart;}
static UINT16 FlowLayer(const UDM_WFP_FLOW* row){return row->Protocol==IPPROTO_UDP?(row->Family==AF_INET?FWPS_LAYER_DATAGRAM_DATA_V4:FWPS_LAYER_DATAGRAM_DATA_V6):(row->Family==AF_INET?FWPS_LAYER_STREAM_V4:FWPS_LAYER_STREAM_V6);}
static UINT32 FlowCallout(const UDM_WFP_FLOW* row){return gCallout[(row->Protocol==IPPROTO_UDP?4:2)+(row->Family==AF_INET?0:1)];}
/* Watched and FindFlow require gLock. */
static BOOLEAN Watched(UINT64 pid){ULONG i;for(i=0;i<gWatchCount;i++)if(gWatch[i]==pid)return TRUE;return FALSE;}
static LONG FindFlow(UINT64 flow){ULONG i;for(i=0;i<UDM_WFP_MAX_FLOWS;i++)if(gFlows[i].FlowId==flow&&(gFlows[i].Flags&UDM_FLOW_ACTIVE))return (LONG)i;return -1;}
static void CloseFlow(UINT64 flow){KIRQL irql;LONG slot;KeAcquireSpinLock(&gLock,&irql);slot=FindFlow(flow);if(slot>=0){gFlows[slot].Flags=UDM_FLOW_CLOSED;gFlows[slot].LastSeenUtc=Now();if(--gFlowRefs==0)KeSetEvent(&gNoFlows,IO_NO_INCREMENT,FALSE);}KeReleaseSpinLock(&gLock,irql);}
static void NTAPI FlowDeleted(UINT16 layerId,UINT32 calloutId,UINT64 context){UNREFERENCED_PARAMETER(layerId);UNREFERENCED_PARAMETER(calloutId);CloseFlow(context);}
static NTSTATUS NTAPI Notify(FWPS_CALLOUT_NOTIFY_TYPE type,const GUID* key,const FWPS_FILTER0* filter){UNREFERENCED_PARAMETER(type);UNREFERENCED_PARAMETER(key);UNREFERENCED_PARAMETER(filter);return STATUS_SUCCESS;}
static void Address4(UINT8* output,UINT32 value){output[0]=(UINT8)(value>>24);output[1]=(UINT8)(value>>16);output[2]=(UINT8)(value>>8);output[3]=(UINT8)value;}
static void ObserveFlow(const FWPS_INCOMING_VALUES0* values,const FWPS_INCOMING_METADATA_VALUES0* metadata){
    UDM_WFP_FLOW row={0};KIRQL irql;LONG slot=-1;ULONG i;UINT16 streamLayer;UINT32 streamCallout;NTSTATUS status;BOOLEAN v4;
    if(!FWPS_IS_METADATA_FIELD_PRESENT(metadata,FWPS_METADATA_FIELD_PROCESS_ID)||!FWPS_IS_METADATA_FIELD_PRESENT(metadata,FWPS_METADATA_FIELD_FLOW_HANDLE)||metadata->flowHandle==0)return;
    v4=values->layerId==FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4;
    row.Protocol=values->incomingValue[v4?FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_PROTOCOL:FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_PROTOCOL].value.uint8;
    if(row.Protocol!=IPPROTO_TCP&&row.Protocol!=IPPROTO_UDP)return;
    row.ProcessId=metadata->processId;row.FlowId=metadata->flowHandle;row.Family=v4?AF_INET:AF_INET6;row.Flags=UDM_FLOW_ACTIVE;row.OpenedUtc=row.LastSeenUtc=Now();
    row.LocalPort=values->incomingValue[v4?FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_LOCAL_PORT:FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_LOCAL_PORT].value.uint16;
    row.RemotePort=values->incomingValue[v4?FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_REMOTE_PORT:FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_REMOTE_PORT].value.uint16;
    if(v4){Address4(row.LocalAddress,values->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_LOCAL_ADDRESS].value.uint32);Address4(row.RemoteAddress,values->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_REMOTE_ADDRESS].value.uint32);}
    else{RtlCopyMemory(row.LocalAddress,values->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_LOCAL_ADDRESS].value.byteArray16->byteArray16,16);RtlCopyMemory(row.RemoteAddress,values->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_REMOTE_ADDRESS].value.byteArray16->byteArray16,16);}
    KeAcquireSpinLock(&gLock,&irql);
    if(!Watched(row.ProcessId)||FindFlow(row.FlowId)>=0){KeReleaseSpinLock(&gLock,irql);return;}
    for(i=0;i<UDM_WFP_MAX_FLOWS;i++)if(!(gFlows[i].Flags&UDM_FLOW_ACTIVE)&&(slot<0||gFlows[i].LastSeenUtc<gFlows[slot].LastSeenUtc))slot=(LONG)i;
    if(slot<0){gDropped++;KeReleaseSpinLock(&gLock,irql);return;}
    gFlows[slot]=row;gConnections++;gFlowRefs++;KeClearEvent(&gNoFlows);KeReleaseSpinLock(&gLock,irql);
    streamLayer=FlowLayer(&row);streamCallout=FlowCallout(&row);
    status=FwpsFlowAssociateContext0(row.FlowId,streamLayer,streamCallout,row.FlowId);
    if(!NT_SUCCESS(status)){CloseFlow(row.FlowId);KeAcquireSpinLock(&gLock,&irql);gDropped++;KeReleaseSpinLock(&gLock,irql);}
}
static void NTAPI Classify(const FWPS_INCOMING_VALUES0* values,const FWPS_INCOMING_METADATA_VALUES0* metadata,void* data,const FWPS_FILTER0* filter,UINT64 context,FWPS_CLASSIFY_OUT0* result){
    UNREFERENCED_PARAMETER(filter);
    if(result->rights&FWPS_RIGHT_ACTION_WRITE)result->actionType=FWP_ACTION_CONTINUE;
    if(values->layerId==FWPS_LAYER_DATAGRAM_DATA_V4||values->layerId==FWPS_LAYER_DATAGRAM_DATA_V6){
        NET_BUFFER_LIST* list;UINT64 bytes=0;KIRQL irql;LONG slot;UINT32 direction=values->incomingValue[values->layerId==FWPS_LAYER_DATAGRAM_DATA_V4?FWPS_FIELD_DATAGRAM_DATA_V4_DIRECTION:FWPS_FIELD_DATAGRAM_DATA_V6_DIRECTION].value.uint32;
        if(gStopping||!ExAcquireRundownProtection(&gRundown))return;
        for(list=(NET_BUFFER_LIST*)data;list;list=NET_BUFFER_LIST_NEXT_NBL(list)){NET_BUFFER* buffer;for(buffer=NET_BUFFER_LIST_FIRST_NB(list);buffer;buffer=NET_BUFFER_NEXT_NB(buffer))bytes+=NET_BUFFER_DATA_LENGTH(buffer);}
        KeAcquireSpinLock(&gLock,&irql);slot=FindFlow(context);if(slot>=0&&Watched(gFlows[slot].ProcessId)){if(direction==FWP_DIRECTION_INBOUND){gFlows[slot].ReceivedBytes+=bytes;gReceived+=bytes;}else{gFlows[slot].SentBytes+=bytes;gSent+=bytes;}gFlows[slot].LastSeenUtc=Now();}KeReleaseSpinLock(&gLock,irql);ExReleaseRundownProtection(&gRundown);return;
    }
    if(values->layerId==FWPS_LAYER_STREAM_V4||values->layerId==FWPS_LAYER_STREAM_V6){
        FWPS_STREAM_CALLOUT_IO_PACKET0* packet=(FWPS_STREAM_CALLOUT_IO_PACKET0*)data;
        if(packet&&packet->streamData){KIRQL irql;LONG slot;UINT64 bytes=packet->streamData->dataLength;
            packet->streamAction=FWPS_STREAM_ACTION_NONE;packet->countBytesRequired=0;packet->countBytesEnforced=packet->streamData->dataLength;
            if(gStopping||!ExAcquireRundownProtection(&gRundown))return;
            KeAcquireSpinLock(&gLock,&irql);slot=FindFlow(context);
            if(slot>=0&&Watched(gFlows[slot].ProcessId)){if(packet->streamData->flags&FWPS_STREAM_FLAG_RECEIVE){gFlows[slot].ReceivedBytes+=bytes;gReceived+=bytes;}else if(packet->streamData->flags&FWPS_STREAM_FLAG_SEND){gFlows[slot].SentBytes+=bytes;gSent+=bytes;}gFlows[slot].LastSeenUtc=Now();if(packet->missedBytes)gDropped++;}
            KeReleaseSpinLock(&gLock,irql);ExReleaseRundownProtection(&gRundown);
        }return;
    }
    if(gStopping||!ExAcquireRundownProtection(&gRundown))return;
    ObserveFlow(values,metadata);ExReleaseRundownProtection(&gRundown);
}
static NTSTATUS Complete(PIRP irp,NTSTATUS status,ULONG_PTR bytes){irp->IoStatus.Status=status;irp->IoStatus.Information=bytes;IoCompleteRequest(irp,IO_NO_INCREMENT);return status;}
_Use_decl_annotations_
static NTSTATUS Dispatch(PDEVICE_OBJECT device,PIRP irp){
    PIO_STACK_LOCATION stack=IoGetCurrentIrpStackLocation(irp);KIRQL irql;NTSTATUS status=STATUS_INVALID_DEVICE_REQUEST;ULONG_PTR written=0;ULONG i;
    UNREFERENCED_PARAMETER(device);
    if(gStopping)return Complete(irp,STATUS_DELETE_PENDING,0);
    if(stack->MajorFunction==IRP_MJ_CREATE){if(stack->FileObject->FileName.Length!=0)return Complete(irp,STATUS_OBJECT_NAME_NOT_FOUND,0);return Complete(irp,STATUS_SUCCESS,0);}
    if(stack->MajorFunction==IRP_MJ_CLEANUP){KeAcquireSpinLock(&gLock,&irql);gWatchCount=0;gGeneration++;KeReleaseSpinLock(&gLock,irql);return Complete(irp,STATUS_SUCCESS,0);}
    if(stack->MajorFunction==IRP_MJ_CLOSE)return Complete(irp,STATUS_SUCCESS,0);
    if(stack->MajorFunction!=IRP_MJ_DEVICE_CONTROL)return Complete(irp,status,0);
    if(!ExAcquireRundownProtection(&gRundown))return Complete(irp,STATUS_DELETE_PENDING,0);
    if(stack->Parameters.DeviceIoControl.IoControlCode==IOCTL_UDM_WFP_WATCH){
        UDM_WFP_WATCH* request=(UDM_WFP_WATCH*)irp->AssociatedIrp.SystemBuffer;status=STATUS_INVALID_PARAMETER;
        if(request&&stack->Parameters.DeviceIoControl.InputBufferLength==sizeof(*request)&&request->Version==UDM_WFP_VERSION&&request->Size==sizeof(*request)&&request->Count<=UDM_WFP_MAX_PIDS&&request->Reserved==0){
            BOOLEAN valid=TRUE;for(i=0;i<request->Count;i++)if(request->ProcessIds[i]<=4)valid=FALSE;
            if(valid){KeAcquireSpinLock(&gLock,&irql);gWatchCount=request->Count;RtlCopyMemory(gWatch,request->ProcessIds,gWatchCount*sizeof(UINT32));gGeneration++;KeReleaseSpinLock(&gLock,irql);status=STATUS_SUCCESS;}
        }
    }else if(stack->Parameters.DeviceIoControl.IoControlCode==IOCTL_UDM_WFP_SNAPSHOT){
        ULONG available=stack->Parameters.DeviceIoControl.OutputBufferLength;UDM_WFP_SNAPSHOT* reply=(UDM_WFP_SNAPSHOT*)irp->AssociatedIrp.SystemBuffer;status=STATUS_BUFFER_TOO_SMALL;
        if(stack->Parameters.DeviceIoControl.InputBufferLength!=0)status=STATUS_INVALID_PARAMETER;
        else if(reply&&available>=sizeof(*reply)+UDM_WFP_MAX_FLOWS*sizeof(UDM_WFP_FLOW)){
            UDM_WFP_FLOW* rows=(UDM_WFP_FLOW*)(reply+1);RtlZeroMemory(reply,sizeof(*reply)+UDM_WFP_MAX_FLOWS*sizeof(UDM_WFP_FLOW));reply->Version=UDM_WFP_VERSION;reply->HeaderSize=sizeof(*reply);reply->RecordSize=sizeof(UDM_WFP_FLOW);
            KeAcquireSpinLock(&gLock,&irql);reply->Generation=gGeneration;reply->DroppedFlows=gDropped;reply->Connections=gConnections;reply->ReceivedBytes=gReceived;reply->SentBytes=gSent;reply->WatchedProcesses=gWatchCount;
            for(i=0;i<UDM_WFP_MAX_FLOWS;i++)if(gFlows[i].FlowId&&Watched(gFlows[i].ProcessId))rows[reply->Count++]=gFlows[i];
            KeReleaseSpinLock(&gLock,irql);written=sizeof(*reply)+reply->Count*sizeof(UDM_WFP_FLOW);status=STATUS_SUCCESS;
        }
    }
    ExReleaseRundownProtection(&gRundown);return Complete(irp,status,written);
}
static NTSTATUS RegisterCallout(const GUID* key,const GUID* layer,ULONG index){
    FWPS_CALLOUT0 runtime={0};FWPM_CALLOUT0 management={0};FWPM_FILTER0 filter={0};NTSTATUS status;
    runtime.calloutKey=*key;runtime.classifyFn=Classify;runtime.notifyFn=Notify;if(index>=2)runtime.flowDeleteFn=FlowDeleted;
    status=FwpsCalloutRegister0(gDevice,&runtime,&gCallout[index]);if(!NT_SUCCESS(status))return status;
    management.calloutKey=*key;management.displayData.name=L"UDM scoped network monitor";management.applicableLayer=*layer;status=FwpmCalloutAdd0(gEngine,&management,NULL,NULL);if(!NT_SUCCESS(status))return status;
    filter.displayData.name=L"UDM non-blocking network inspection";filter.layerKey=*layer;filter.subLayerKey=UDM_SUBLAYER;filter.weight.type=FWP_EMPTY;filter.action.type=FWP_ACTION_CALLOUT_INSPECTION;filter.action.calloutKey=*key;
    return FwpmFilterAdd0(gEngine,&filter,NULL,NULL);
}
static void Cleanup(void){
    ULONG i;UNICODE_STRING link=RTL_CONSTANT_STRING(L"\\DosDevices\\UdmWfp");InterlockedExchange(&gStopping,1);
    if(gEngine){FwpmEngineClose0(gEngine);gEngine=NULL;}ExWaitForRundownProtectionRelease(&gRundown);
    for(i=0;i<UDM_WFP_MAX_FLOWS;i++){UDM_WFP_FLOW row;KIRQL irql;KeAcquireSpinLock(&gLock,&irql);row=gFlows[i];KeReleaseSpinLock(&gLock,irql);if(row.Flags&UDM_FLOW_ACTIVE)FwpsFlowRemoveContext0(row.FlowId,FlowLayer(&row),FlowCallout(&row));}
    KeWaitForSingleObject(&gNoFlows,Executive,KernelMode,FALSE,NULL);
    for(i=0;i<6;i++)if(gCallout[i]){
        /* Never unload code while WFP still owns its callback address. */
        for(;;){NTSTATUS status=FwpsCalloutUnregisterById0(gCallout[i]);LARGE_INTEGER delay;if(NT_SUCCESS(status)||status==STATUS_FWP_CALLOUT_NOT_FOUND)break;delay.QuadPart=-100000;KeDelayExecutionThread(KernelMode,FALSE,&delay);}gCallout[i]=0;
    }
    if(gLinkCreated){IoDeleteSymbolicLink(&link);gLinkCreated=FALSE;}if(gDevice){IoDeleteDevice(gDevice);gDevice=NULL;}
}
_Use_decl_annotations_
static void Unload(PDRIVER_OBJECT driver){UNREFERENCED_PARAMETER(driver);Cleanup();}
DRIVER_INITIALIZE DriverEntry;
NTSTATUS DriverEntry(PDRIVER_OBJECT driver,PUNICODE_STRING registryPath){
    FWPM_SESSION0 session={0};FWPM_SUBLAYER0 sublayer={0};NTSTATUS status;BOOLEAN transaction=FALSE;ULONG i;
    UNICODE_STRING name=RTL_CONSTANT_STRING(L"\\Device\\UdmWfp"),link=RTL_CONSTANT_STRING(L"\\DosDevices\\UdmWfp"),security=RTL_CONSTANT_STRING(L"D:P(A;;GA;;;SY)(A;;GA;;;BA)");
    UNREFERENCED_PARAMETER(registryPath);KeInitializeSpinLock(&gLock);ExInitializeRundownProtection(&gRundown);KeInitializeEvent(&gNoFlows,NotificationEvent,TRUE);
    driver->DriverUnload=Unload;for(i=0;i<=IRP_MJ_MAXIMUM_FUNCTION;i++)driver->MajorFunction[i]=Dispatch;
    status=IoCreateDeviceSecure(driver,0,&name,UDM_WFP_DEVICE_TYPE,FILE_DEVICE_SECURE_OPEN,TRUE,&security,&UDM_DEVICE_CLASS,&gDevice);if(!NT_SUCCESS(status))return status;
    session.flags=FWPM_SESSION_FLAG_DYNAMIC;status=FwpmEngineOpen0(NULL,RPC_C_AUTHN_WINNT,NULL,&session,&gEngine);if(!NT_SUCCESS(status))goto failure;
    status=FwpmTransactionBegin0(gEngine,0);if(!NT_SUCCESS(status))goto failure;transaction=TRUE;
    sublayer.subLayerKey=UDM_SUBLAYER;sublayer.displayData.name=L"UDM network monitoring";sublayer.weight=0x100;status=FwpmSubLayerAdd0(gEngine,&sublayer,NULL);if(!NT_SUCCESS(status))goto failure;
    status=RegisterCallout(&UDM_STREAM4,&FWPM_LAYER_STREAM_V4,2);if(!NT_SUCCESS(status))goto failure;
    status=RegisterCallout(&UDM_STREAM6,&FWPM_LAYER_STREAM_V6,3);if(!NT_SUCCESS(status))goto failure;
    status=RegisterCallout(&UDM_DATAGRAM4,&FWPM_LAYER_DATAGRAM_DATA_V4,4);if(!NT_SUCCESS(status))goto failure;
    status=RegisterCallout(&UDM_DATAGRAM6,&FWPM_LAYER_DATAGRAM_DATA_V6,5);if(!NT_SUCCESS(status))goto failure;
    status=RegisterCallout(&UDM_FLOW4,&FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4,0);if(!NT_SUCCESS(status))goto failure;
    status=RegisterCallout(&UDM_FLOW6,&FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6,1);if(!NT_SUCCESS(status))goto failure;
    status=FwpmTransactionCommit0(gEngine);if(!NT_SUCCESS(status))goto failure;transaction=FALSE;
    status=IoCreateSymbolicLink(&link,&name);if(!NT_SUCCESS(status))goto failure;gLinkCreated=TRUE;gDevice->Flags&=~DO_DEVICE_INITIALIZING;return STATUS_SUCCESS;
failure:
    if(transaction)FwpmTransactionAbort0(gEngine);Cleanup();return status;
}
