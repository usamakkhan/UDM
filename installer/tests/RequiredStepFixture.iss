[Setup]
AppId=UDM.RequiredStep.PrivateFixture
AppName=UDM required-step private fixture
AppVersion=1.0
DefaultDirName={tmp}\UDM.RequiredStep.PrivateFixture
CreateAppDir=no
Uninstallable=no
CreateUninstallRegKey=no
PrivilegesRequired=lowest
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableReadyPage=yes
OutputBaseFilename=RequiredStepFixture
Compression=none
[Run]
Filename: "{cmd}"; Parameters: "/D /C exit 0"; Flags: postinstall runhidden; Check: RequiredInstallationSucceeded; BeforeInstall: RecordLaunch
[Code]
#include "..\RequiredStep.iss"
function ExecuteRequiredFixture: Boolean;
var Mode, ProgramPath, Parameters, Receipt: String; Success: Boolean;
begin
  Mode := ExpandConstant('{param:TESTMODE|success}');
  ProgramPath := ExpandConstant('{cmd}');
  if Mode = 'success' then Parameters := '/D /C exit 0'
  else if Mode = 'exit' then Parameters := '/D /C exit 7'
  else if Mode = 'missing' then begin ProgramPath := ExpandConstant('{param:RESULTFILE}') + '.missing.exe'; Parameters := ''; end
  else RaiseException('Unknown private fixture mode.');
  Success := RunRequired(ProgramPath, Parameters, 'UDM private required-step fixture failure');
  if Success then Receipt := 'true' else Receipt := 'false';
  if not SaveStringToFile(ExpandConstant('{param:RESULTFILE}'), AnsiString(Receipt), False) then RaiseException('Cannot save fixture result.');
  Result := Success;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  if ExpandConstant('{param:TESTPHASE|prepare}') = 'prepare' then
    if not ExecuteRequiredFixture then Result := 'The required private fixture step failed.';
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) and (ExpandConstant('{param:TESTPHASE|prepare}') = 'postinstall') then
    if not ExecuteRequiredFixture then RaiseException('Required post-install fixture verification failed.');
end;
procedure RecordLaunch;
begin
  if not SaveStringToFile(ExpandConstant('{param:RESULTFILE}') + '.launch', 'launch', False) then
    RaiseException('Cannot save fixture launch result.');
end;