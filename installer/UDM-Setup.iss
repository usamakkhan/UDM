#ifndef AppVersion
  #define AppVersion "1.0.4"
#endif

#ifndef BrowserVersion
  #define BrowserVersion "0.63.20"
#endif

#ifndef ReleaseRoot
  #define ReleaseRoot "..\release"
#endif
#ifndef BrowserRoot
  #define BrowserRoot "..\browser"
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
#ifdef SignSetup
; build-setup.ps1 defines SignSetup and the "udm" sign tool when a certificate is supplied.
SignTool=udm
SignedUninstaller=yes
#endif

[Files]
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\NOTICE"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ReleaseRoot}\Udm.SetupHelper.exe"; Flags: dontcopy
Source: "{#ReleaseRoot}\UDM.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ReleaseRoot}\Udm.NativeHost.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ReleaseRoot}\Udm.Monitor.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ReleaseRoot}\Udm.Automation.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ReleaseRoot}\Udm.Automation.tlb"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ReleaseRoot}\nghttp2-LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ReleaseRoot}\Brotli-LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ReleaseRoot}\zlib-LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ReleaseRoot}\curl-LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ReleaseRoot}\mime-db-LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ReleaseRoot}\assets\*"; DestDir: "{app}\assets"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#ReleaseRoot}\tools\ffmpeg.exe"; DestDir: "{app}\tools"; Flags: ignoreversion
Source: "{#ReleaseRoot}\tools\ffprobe.exe"; DestDir: "{app}\tools"; Flags: ignoreversion
Source: "{#ReleaseRoot}\tools\FFmpeg-LICENSE.txt"; DestDir: "{app}\tools"; Flags: ignoreversion
Source: "{#BrowserRoot}\chromium\*"; DestDir: "{app}\browser\chromium"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BrowserRoot}\firefox\*"; DestDir: "{app}\browser\firefox"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BrowserRoot}\extension-setup.html"; DestDir: "{app}\browser"; Flags: ignoreversion

Source: "{#ReleaseRoot}\network\Udm.Network.exe"; DestDir: "{app}\network"; Flags: ignoreversion
Source: "{#ReleaseRoot}\network\runtime\WinDivert.dll"; DestDir: "{app}\network\runtime"; Flags: ignoreversion
Source: "{#ReleaseRoot}\network\runtime\WinDivert64.sys"; DestDir: "{app}\network\runtime"; Flags: ignoreversion
Source: "{#ReleaseRoot}\network\runtime\WinDivert-LICENSE.txt"; DestDir: "{app}\network\runtime"; Flags: ignoreversion
Source: "{#ReleaseRoot}\network\WinDivert-2.2.2-Source.zip"; DestDir: "{app}\network"; Flags: ignoreversion
Source: "{#ReleaseRoot}\network\README.md"; DestDir: "{app}\network"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\UDM Download Manager"; Filename: "{app}\UDM.exe"
Name: "{autodesktop}\UDM Download Manager"; Filename: "{app}\UDM.exe"

[Registry]
; Explorer: "Download with UDM" on Internet Shortcut (.url) files. The verb hands the shortcut to UDM.exe --shortcut,
; which reads the address from the file; removed with the application.
Root: HKA; Subkey: "Software\Classes\InternetShortcut\shell\UDM.Download"; ValueType: string; ValueName: ""; ValueData: "Download with UDM"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\InternetShortcut\shell\UDM.Download"; ValueType: string; ValueName: "Icon"; ValueData: """{app}\UDM.exe"",0"
Root: HKA; Subkey: "Software\Classes\InternetShortcut\shell\UDM.Download\command"; ValueType: string; ValueName: ""; ValueData: """{app}\UDM.exe"" --shortcut ""%1"""

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
