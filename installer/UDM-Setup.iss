#ifndef AppVersion
  #define AppVersion "1.0.3"
#endif

#ifndef BrowserVersion
  #define BrowserVersion "0.63.17"
#endif

[Setup]
AppId={{C8A0DA4B-B9A2-4AF2-B058-1797825BB53E}
AppName=UDM Download Manager
AppVersion={#AppVersion}
AppPublisher=UDM Project
DefaultDirName={autopf}\UDM
DefaultGroupName=UDM Download Manager
OutputDir=..\installer-out
OutputBaseFilename=UDM-{#AppVersion}-Browser-{#BrowserVersion}-Setup-x64
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
DisableProgramGroupPage=yes
UninstallDisplayName=UDM Download Manager
RestartIfNeededByRun=no

[Files]
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\NOTICE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\release\Udm.SetupHelper.exe"; Flags: dontcopy
Source: "..\release\UDM.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\release\Udm.NativeHost.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\release\Udm.Monitor.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\release\Udm.Automation.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\release\Udm.Automation.tlb"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\release\nghttp2-LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\release\Brotli-LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\release\zlib-LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\release\curl-LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\release\assets\*"; DestDir: "{app}\assets"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\release\tools\ffmpeg.exe"; DestDir: "{app}\tools"; Flags: ignoreversion
Source: "..\release\tools\ffprobe.exe"; DestDir: "{app}\tools"; Flags: ignoreversion
Source: "..\release\tools\FFmpeg-LICENSE.txt"; DestDir: "{app}\tools"; Flags: ignoreversion
Source: "..\browser\chromium\*"; DestDir: "{app}\browser\chromium"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\browser\firefox\*"; DestDir: "{app}\browser\firefox"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\browser\extension-setup.html"; DestDir: "{app}\browser"; Flags: ignoreversion

Source: "..\release\network\Udm.Network.exe"; DestDir: "{app}\network"; Flags: ignoreversion
Source: "..\release\network\runtime\WinDivert.dll"; DestDir: "{app}\network\runtime"; Flags: ignoreversion
Source: "..\release\network\runtime\WinDivert64.sys"; DestDir: "{app}\network\runtime"; Flags: ignoreversion
Source: "..\release\network\runtime\WinDivert-LICENSE.txt"; DestDir: "{app}\network\runtime"; Flags: ignoreversion
Source: "..\release\network\WinDivert-2.2.2-Source.zip"; DestDir: "{app}\network"; Flags: ignoreversion
Source: "..\release\network\README.md"; DestDir: "{app}\network"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\UDM Download Manager"; Filename: "{app}\UDM.exe"
Name: "{autodesktop}\UDM Download Manager"; Filename: "{app}\UDM.exe"

[Run]
; Download automation (COM) is registered machine-wide by the elevated installer.
Filename: "{app}\Udm.Automation.exe"; Parameters: "--register --machine"; StatusMsg: "Registering download automation..."; Flags: runhidden waituntilterminated
Filename: "{app}\UDM.exe"; Description: "Launch UDM Download Manager"; Check: RequiredInstallationSucceeded; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{app}\Udm.Automation.exe"; Parameters: "--unregister --machine"; RunOnceId: "UdmAutomationUnregister"; Flags: runhidden waituntilterminated

[Code]
#include "NativeMessaging.iss"
#include "DataMigration.iss"
#include "InstallerLifecycle.iss"

#include "RequiredStep.iss"

procedure CurStepChanged(CurStep: TSetupStep);
var ResultCode: Integer;
begin
  if CurStep = ssPostInstall then begin
    BeginRequiredInstallation;
    if not RunRequired(ExpandConstant('{app}\network\Udm.Network.exe'), '--status', 'The signed network runtime failed verification.') then
      RaiseException('Network runtime verification failed.');
    ExtractTemporaryFile('Udm.SetupHelper.exe');
    InstallNativeMessagingWithData(ExpandConstant('{tmp}\Udm.SetupHelper.exe'), ExpandConstant('{app}'), ExpandConstant('{localappdata}\UDM'), ExpandConstant('{tmp}\udm-data-request.json'), ExpandConstant('{tmp}\udm-data-receipt.json'));
    CompleteRequiredInstallation;
    if not WizardSilent then
      if not ShellExec('open', ExpandConstant('{app}\browser\extension-setup.html'), '', '', SW_SHOWNORMAL, ewNoWait, ResultCode) then
        Log('Could not open the browser extension setup guide: ' + SysErrorMessage(ResultCode));
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  NativeUninstallLifecycleStep(CurUninstallStep);
  if CurUninstallStep = usUninstall then
    RemoveNativeMessagingRegistration(ExpandConstant('{app}\native-messaging'), ExpandConstant('{app}\Udm.NativeHost.exe'));
end;
