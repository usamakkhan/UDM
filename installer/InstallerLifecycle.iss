// Hold the same transaction lock before file copying/removal, until process teardown.
var NativeLifecycleLock: THandle;
function NativeBeginLifecycle(): Boolean;
begin
 Result := False;
 try NativeLifecycleLock := NativeAcquireTransaction(); Result := True;
 except SuppressibleMsgBox(GetExceptionMessage,mbError,MB_OK,IDOK); end;
end;
procedure NativeEndLifecycle();
var Handle: THandle;
begin
 Handle:=NativeLifecycleLock;NativeLifecycleLock:=0;
 if Handle<>0 then NativeReleaseTransaction(Handle);
end;
function InitializeSetup(): Boolean;
begin Result:=NativeBeginLifecycle();end;
procedure DeinitializeSetup();
begin NativeEndLifecycle();end;
function InitializeUninstall(): Boolean;
begin Result:=NativeBeginLifecycle();end;
procedure NativeUninstallLifecycleStep(CurStep: TUninstallStep);
begin if CurStep=usDone then NativeEndLifecycle();end;
procedure DeinitializeUninstall();
begin NativeEndLifecycle();end;
