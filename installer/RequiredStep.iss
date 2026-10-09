var
  RequiredInstallationFailed: Boolean;

function RunRequired(const FileName, Parameters, Failure: String): Boolean;
var
  ResultCode: Integer;
begin
  Result := Exec(FileName, Parameters, '', SW_HIDE, ewWaitUntilTerminated, ResultCode) and (ResultCode = 0);
  if not Result then begin
    RequiredInstallationFailed := True;
    SuppressibleMsgBox(Failure + #13#10#13#10 + 'Setup cannot continue.', mbError, MB_OK, IDOK);
  end;
end;


function RequiredInstallationSucceeded: Boolean;
begin
  Result := not RequiredInstallationFailed;
end;

function GetCustomSetupExitCode: Integer;
begin
  if RequiredInstallationFailed then Result := 20 else Result := 0;
end;