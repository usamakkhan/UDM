/* Original UDM research source. Unbuilt and unvalidated. Not part of the desktop release.
 * Counts IPv4/IPv6 connection classifications. Does not capture URLs, decrypt TLS,
 * block traffic, redirect connections, inject code, or take over downloads.
 */
#include <ntddk.h>
#include <fwpsk.h>
#include <fwpmk.h>
#include <initguid.h>

DEFINE_GUID(UDM_SUBLAYER, 0xb21d9c81,0x4f26,0x4c44,0xa7,0xf4,0x61,0x09,0xf0,0x51,0xa8,0x22);
DEFINE_GUID(UDM_CONNECT_V4,0xb21d9c82,0x4f26,0x4c44,0xa7,0xf4,0x61,0x09,0xf0,0x51,0xa8,0x22);
DEFINE_GUID(UDM_CONNECT_V6,0xb21d9c83,0x4f26,0x4c44,0xa7,0xf4,0x61,0x09,0xf0,0x51,0xa8,0x22);

static PDEVICE_OBJECT gDevice;
static HANDLE gEngine;
static UINT32 gCallout4, gCallout6;
static volatile LONG64 gClassifications;

static void NTAPI UdmClassify(const FWPS_INCOMING_VALUES0* values,
    const FWPS_INCOMING_METADATA_VALUES0* metadata, void* data,
    const FWPS_FILTER0* filter, UINT64 context, FWPS_CLASSIFY_OUT0* result)
{
    UNREFERENCED_PARAMETER(values); UNREFERENCED_PARAMETER(metadata);
    UNREFERENCED_PARAMETER(data); UNREFERENCED_PARAMETER(filter);
    UNREFERENCED_PARAMETER(context);
    InterlockedIncrement64(&gClassifications);
    if (result->rights & FWPS_RIGHT_ACTION_WRITE) result->actionType = FWP_ACTION_CONTINUE;
}

static NTSTATUS NTAPI UdmNotify(FWPS_CALLOUT_NOTIFY_TYPE type, const GUID* key, const FWPS_FILTER0* filter)
{
    UNREFERENCED_PARAMETER(type); UNREFERENCED_PARAMETER(key); UNREFERENCED_PARAMETER(filter);
    return STATUS_SUCCESS;
}

static NTSTATUS AddCallout(const GUID* key, const GUID* layer, UINT32* id)
{
    FWPS_CALLOUT0 runtime = {0};
    FWPM_CALLOUT0 management = {0};
    FWPM_FILTER0 filter = {0};
    NTSTATUS status;
    runtime.calloutKey = *key;
    runtime.classifyFn = UdmClassify;
    runtime.notifyFn = UdmNotify;
    status = FwpsCalloutRegister0(gDevice, &runtime, id);
    if (!NT_SUCCESS(status)) return status;
    management.calloutKey = *key;
    management.displayData.name = L"UDM connection observer (research)";
    management.applicableLayer = *layer;
    status = FwpmCalloutAdd0(gEngine, &management, NULL, NULL);
    if (!NT_SUCCESS(status)) return status;
    filter.displayData.name = L"UDM non-blocking connection observer";
    filter.layerKey = *layer;
    filter.subLayerKey = UDM_SUBLAYER;
    filter.weight.type = FWP_EMPTY;
    filter.action.type = FWP_ACTION_CALLOUT_INSPECTION;
    filter.action.calloutKey = *key;
    return FwpmFilterAdd0(gEngine, &filter, NULL, NULL);
}

static void Cleanup(void)
{
    // Closing the dynamic session removes management filters before runtime callouts.
    if (gEngine) { FwpmEngineClose0(gEngine); gEngine = NULL; }
    if (gCallout4) { FwpsCalloutUnregisterById0(gCallout4); gCallout4 = 0; }
    if (gCallout6) { FwpsCalloutUnregisterById0(gCallout6); gCallout6 = 0; }
    if (gDevice) { IoDeleteDevice(gDevice); gDevice = NULL; }
}

static void UdmUnload(PDRIVER_OBJECT driver)
{
    UNREFERENCED_PARAMETER(driver);
    Cleanup();
}

DRIVER_INITIALIZE DriverEntry;
NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING registryPath)
{
    FWPM_SESSION0 session = {0};
    FWPM_SUBLAYER0 sublayer = {0};
    NTSTATUS status;
    BOOLEAN transaction = FALSE;
    UNREFERENCED_PARAMETER(registryPath);
    driver->DriverUnload = UdmUnload;
    // No named device, symbolic link, IOCTL, or user-mode control surface in this prototype.
    status = IoCreateDevice(driver, 0, NULL, FILE_DEVICE_NETWORK, FILE_DEVICE_SECURE_OPEN, FALSE, &gDevice);
    if (!NT_SUCCESS(status)) return status;
    gDevice->Flags &= ~DO_DEVICE_INITIALIZING;
    session.flags = FWPM_SESSION_FLAG_DYNAMIC;
    status = FwpmEngineOpen0(NULL, RPC_C_AUTHN_WINNT, NULL, &session, &gEngine);
    if (!NT_SUCCESS(status)) goto failure;
    status = FwpmTransactionBegin0(gEngine, 0);
    if (!NT_SUCCESS(status)) goto failure;
    transaction = TRUE;
    sublayer.subLayerKey = UDM_SUBLAYER;
    sublayer.displayData.name = L"UDM research sublayer";
    sublayer.weight = 0x100;
    status = FwpmSubLayerAdd0(gEngine, &sublayer, NULL);
    if (!NT_SUCCESS(status)) goto failure;
    status = AddCallout(&UDM_CONNECT_V4, &FWPM_LAYER_ALE_AUTH_CONNECT_V4, &gCallout4);
    if (!NT_SUCCESS(status)) goto failure;
    status = AddCallout(&UDM_CONNECT_V6, &FWPM_LAYER_ALE_AUTH_CONNECT_V6, &gCallout6);
    if (!NT_SUCCESS(status)) goto failure;
    status = FwpmTransactionCommit0(gEngine);
    if (!NT_SUCCESS(status)) goto failure;
    return STATUS_SUCCESS;
failure:
    if (transaction) FwpmTransactionAbort0(gEngine);
    Cleanup();
    return status;
}
