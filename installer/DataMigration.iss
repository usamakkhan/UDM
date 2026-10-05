// Request generation and transaction orchestration shared with the compiled installer fixture.
function NativeDataRequest(Installation, Fallback: String): String;
var Browser, Scope: Integer; Manifest, Manifests: String; Found: Boolean;
begin
  Manifests := '';
  for Browser := 0 to 3 do begin
    Found := False;
    for Scope := 0 to 2 do if not Found then begin
      if RegValueExists(NativeRoot(Scope), NativeKey(Scope, Browser), '') then begin
        if not RegQueryStringValue(NativeRoot(Scope), NativeKey(Scope, Browser), '', Manifest) or (Manifest = '') then
          RaiseException('The previous browser host registration cannot be read.');
        if Manifests <> '' then Manifests := Manifests + ',';
        Manifests := Manifests + '"' + NativeJson(Manifest) + '"';
        Found := True;
      end;
    end;
  end;
  Result := '{"installationDirectory":"' + NativeJson(Installation) + '","fallbackDirectory":"' + NativeJson(Fallback) + '","manifests":[' + Manifests + ']}';
end;

procedure RunNativeDataHelper(Helper, Parameters: String);
var ExitCode, I: Integer; Output: TExecOutput; Started: Boolean; Failure: String;
begin
  Started := ExecAndCaptureOutput(Helper, Parameters, '', SW_HIDE, ewWaitUntilTerminated, ExitCode, Output);
  for I := 0 to GetArrayLength(Output.StdOut)-1 do Log(Output.StdOut[I]);
  Failure := '';
  for I := 0 to GetArrayLength(Output.StdErr)-1 do begin
    Log(Output.StdErr[I]);
    if Length(Failure) < 2048 then Failure := Failure + Output.StdErr[I] + #13#10;
  end;
  if not Started or (ExitCode <> 0) or Output.Error then begin
    if Failure = '' then Failure := 'The existing download history location could not be preserved.';
    RaiseException(Trim(Failure));
  end;
end;

procedure InstallNativeMessagingWithDataUnlocked(Helper, Installation, Fallback, RequestFile, ReceiptFile: String);
var Failure: String;
begin
  NativePublishManifest(RequestFile, NativeDataRequest(Installation, Fallback));
  try
    RunNativeDataHelper(Helper, '--prepare-data "' + RequestFile + '" "' + ReceiptFile + '"');
    WriteNativeMessagingRegistration(AddBackslash(Installation) + 'native-messaging', AddBackslash(Installation) + 'Udm.NativeHost.exe');
  except
    Failure := GetExceptionMessage;
    if FileExists(ReceiptFile) then begin
      try RunNativeDataHelper(Helper, '--rollback-data "' + ReceiptFile + '"');
      except Failure := Failure + #13#10 + 'Data-location rollback: ' + GetExceptionMessage; end;
    end;
    RaiseException(Failure);
  end;
end;

procedure InstallNativeMessagingWithData(Helper, Installation, Fallback, RequestFile, ReceiptFile: String);
var Handle: THandle;
begin
 Handle:=NativeAcquireTransaction();
 try InstallNativeMessagingWithDataUnlocked(Helper,Installation,Fallback,RequestFile,ReceiptFile);
 finally NativeReleaseTransaction(Handle);end;
end;