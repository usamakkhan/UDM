// One transaction across cooperating setup/uninstall processes and registry scopes.
function NativeCreateMutex(Attributes: LongWord; InitialOwner: Boolean; Name: String): THandle;
 external 'CreateMutexW@kernel32.dll stdcall';
function NativeWaitMutex(Handle: THandle; Milliseconds: Cardinal): Cardinal;
 external 'WaitForSingleObject@kernel32.dll stdcall';
function NativeReleaseMutex(Handle: THandle): Boolean;
 external 'ReleaseMutex@kernel32.dll stdcall';
function NativeCloseMutex(Handle: THandle): Boolean;
 external 'CloseHandle@kernel32.dll stdcall';
#ifdef UDM_NATIVE_REGISTRATION_TEST
var NativeTestLastMutexWait: Cardinal;
#endif
function NativeTransactionMutexName(): String;
begin
#ifdef UDM_NATIVE_REGISTRATION_TEST
 Result := ExpandConstant('{param:UDMLOCKNAME|Local\UDMInstallerPrivateFixtures}');
#else
 Result := 'Global\UDM.Installation.Transaction.v1';
#endif
end;
function NativeAcquireTransaction(): THandle;
var Handle: THandle; Status: Cardinal;
begin
 Handle := NativeCreateMutex(0,False,NativeTransactionMutexName());
 if Handle=0 then RaiseException('Cannot create the UDM setup or uninstall lock. Check permissions and retry.');
 Status := NativeWaitMutex(Handle,0);
#ifdef UDM_NATIVE_REGISTRATION_TEST
 NativeTestLastMutexWait := Status;
#endif
 if (Status=0) or (Status=$80) then begin
  if Status=$80 then Log('Previous UDM transaction owner exited; existing files and registrations will be revalidated.');
  Result := Handle; Exit;
 end;
 NativeCloseMutex(Handle);
 if Status=$102 then RaiseException('Another UDM setup or uninstall is running. Close it and retry.');
 RaiseException('Cannot acquire the UDM setup or uninstall lock. Check permissions and retry.');
end;
procedure NativeReleaseTransaction(Handle: THandle);
begin
 try
  if not NativeReleaseMutex(Handle) then RaiseException('Cannot release the UDM installation transaction lock.');
 finally NativeCloseMutex(Handle); end;
end;