#pragma once
/* Fixed-width ABI: no pointers, kernel addresses, strings or payload data. */
#define UDM_WFP_VERSION 1u
#define UDM_WFP_MAX_PIDS 32u
#define UDM_WFP_MAX_FLOWS 128u
#define UDM_WFP_DEVICE_TYPE 0x8000u
#define IOCTL_UDM_WFP_WATCH CTL_CODE(UDM_WFP_DEVICE_TYPE,0x800,METHOD_BUFFERED,FILE_WRITE_DATA)
#define IOCTL_UDM_WFP_SNAPSHOT CTL_CODE(UDM_WFP_DEVICE_TYPE,0x801,METHOD_BUFFERED,FILE_READ_DATA)
#define UDM_FLOW_ACTIVE 1u
#define UDM_FLOW_CLOSED 2u
#pragma pack(push,8)
typedef struct _UDM_WFP_WATCH {UINT32 Version,Size,Count,Reserved;UINT32 ProcessIds[UDM_WFP_MAX_PIDS];} UDM_WFP_WATCH;
typedef struct _UDM_WFP_FLOW {
    UINT64 FlowId,ProcessId,OpenedUtc,LastSeenUtc,ReceivedBytes,SentBytes;
    UINT32 Family,Protocol;UINT16 LocalPort,RemotePort;UINT32 Flags;
    UINT8 LocalAddress[16],RemoteAddress[16];
} UDM_WFP_FLOW;
typedef struct _UDM_WFP_SNAPSHOT {
    UINT32 Version,HeaderSize,RecordSize,Count;
    UINT64 Generation,DroppedFlows,Connections,ReceivedBytes,SentBytes;
    UINT32 WatchedProcesses,Reserved;
} UDM_WFP_SNAPSHOT;
#pragma pack(pop)
C_ASSERT(sizeof(UDM_WFP_WATCH)==144);
C_ASSERT(sizeof(UDM_WFP_FLOW)==96);
C_ASSERT(sizeof(UDM_WFP_SNAPSHOT)==64);
