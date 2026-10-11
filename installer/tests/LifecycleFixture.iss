#define UDM_NATIVE_REGISTRATION_TEST
[Setup]
AppId=UDMIsolatedLifecycleFixture
AppName=UDM isolated lifecycle fixture
AppVersion=1.0
DefaultDirName={localappdata}\UDMIsolatedLifecycleFixture
PrivilegesRequired=lowest
DisableProgramGroupPage=yes
Uninstallable=yes
CreateUninstallRegKey=no
OutputDir=.
OutputBaseFilename=LifecycleFixture
[Files]
Source: "LifecycleFixture.txt"; DestDir: "{app}"; Flags: ignoreversion
[Code]
#include "..\NativeMessaging.iss"
#include "..\InstallerLifecycle.iss"
procedure CurUninstallStepChanged(CurStep: TUninstallStep);
begin NativeUninstallLifecycleStep(CurStep); end;