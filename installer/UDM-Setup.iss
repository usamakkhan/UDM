#ifndef AppVersion
  #define AppVersion "0.31.0"
#endif
#ifndef DriverThumbprint
  #define DriverThumbprint "A0C6348A0B895699A93639A993432B193852D462"
#endif

[Setup]
AppId={{C8A0DA4B-B9A2-4AF2-B058-1797825BB53E}
AppName=UDM Download Manager
AppVersion={#AppVersion}
AppPublisher=UDM Project
DefaultDirName={autopf}\UDM
DefaultGroupName=UDM Download Manager
OutputDir=..\installer-out
OutputBaseFilename=UDM-{#AppVersion}-Setup-x64
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
DisableProgramGroupPage=yes
UninstallDisplayName=UDM Download Manager
RestartIfNeededByRun=no

[Files]
Source: "..\release\UDM.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\release\Udm.NativeHost.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\release\Udm.Monitor.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\release\assets\*"; DestDir: "{app}\assets"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\release\tools\ffmpeg.exe"; DestDir: "{app}\tools"; Flags: ignoreversion
Source: "..\release\tools\ffprobe.exe"; DestDir: "{app}\tools"; Flags: ignoreversion
Source: "..\release\tools\FFmpeg-LICENSE.txt"; DestDir: "{app}\tools"; Flags: ignoreversion
Source: "..\browser\chromium\*"; DestDir: "{app}\browser\chromium"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\browser\firefox\*"; DestDir: "{app}\browser\firefox"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\drivers\out\x64-test-signed\UdmWfp.sys"; DestDir: "{app}\driver"; Flags: ignoreversion
Source: "..\drivers\out\x64-test-signed\UdmWfp.inf"; DestDir: "{app}\driver"; Flags: ignoreversion
Source: "..\drivers\out\x64-test-signed\udmwfp.cat"; DestDir: "{app}\driver"; Flags: ignoreversion
Source: "..\drivers\out\x64-test-signed\UDM-development-public.cer"; DestDir: "{app}\driver"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\UDM Download Manager"; Filename: "{app}\UDM.exe"
Name: "{autodesktop}\UDM Download Manager"; Filename: "{app}\UDM.exe"

[Run]
Filename: "{app}\UDM.exe"; Description: "Launch UDM Download Manager"; Flags: nowait postinstall skipifsilent

[Code]
const
  DriverThumbprint = '{#DriverThumbprint}';
  HostName = 'com.udm.download_manager';
  ChromiumId = 'kahfappnpjdcboccpnhinkcobcdgbdpl';

function JsonPath(Value: String): String;
begin
  StringChangeEx(Value, '\', '\\', True);
  Result := Value;
end;

function RunRequired(const FileName, Parameters, Failure: String): Boolean;
var
  ResultCode: Integer;
begin
  Result := Exec(FileName, Parameters, '', SW_HIDE, ewWaitUntilTerminated, ResultCode) and (ResultCode = 0);
  if not Result then
    MsgBox(Failure + #13#10#13#10 + 'Setup cannot continue.', mbError, MB_OK);
end;

function InitializeSetup(): Boolean;
begin
  Result := IsWin64;
  if not Result then begin
    MsgBox('UDM Setup requires 64-bit Windows.', mbError, MB_OK);
    exit;
  end;
  Result := MsgBox('This setup installs the UDM WFP development driver. It enables Windows Test Mode and requires a restart. Windows may refuse Test Mode while Secure Boot is enabled. Continue only if you accept these system-wide changes.', mbConfirmation, MB_YESNO) = IDYES;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
begin
  Result := '';
  if not Exec(ExpandConstant('{sys}\bcdedit.exe'), '/set testsigning on', '', SW_HIDE, ewWaitUntilTerminated, ResultCode) or (ResultCode <> 0) then begin
    Result := 'Windows refused to enable Test Mode. Disable Secure Boot if necessary, then run setup again.';
    exit;
  end;
  NeedsRestart := True;
end;

procedure WriteNativeMessagingManifests();
var
  Directory, ChromiumManifest, FirefoxManifest, HostPath: String;
begin
  Directory := ExpandConstant('{commonappdata}\UDM\native-messaging');
  ForceDirectories(Directory);
  HostPath := JsonPath(ExpandConstant('{app}\Udm.NativeHost.exe'));
  ChromiumManifest := AddBackslash(Directory) + 'chromium.json';
  FirefoxManifest := AddBackslash(Directory) + 'firefox.json';
  SaveStringToFile(ChromiumManifest, '{"name":"' + HostName + '","description":"UDM browser download helper","path":"' + HostPath + '","type":"stdio","allowed_origins":["chrome-extension://' + ChromiumId + '/"]}', False);
  SaveStringToFile(FirefoxManifest, '{"name":"' + HostName + '","description":"UDM browser download helper","path":"' + HostPath + '","type":"stdio","allowed_extensions":["udm@local.example"]}', False);
  RegWriteStringValue(HKLM, 'SOFTWARE\Google\Chrome\NativeMessagingHosts\' + HostName, '', ChromiumManifest);
  RegWriteStringValue(HKLM, 'SOFTWARE\Microsoft\Edge\NativeMessagingHosts\' + HostName, '', ChromiumManifest);
  RegWriteStringValue(HKLM, 'SOFTWARE\Chromium\NativeMessagingHosts\' + HostName, '', ChromiumManifest);
  RegWriteStringValue(HKLM, 'SOFTWARE\Mozilla\NativeMessagingHosts\' + HostName, '', FirefoxManifest);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then begin
    if not RunRequired(ExpandConstant('{sys}\certutil.exe'), '-addstore -f root "' + ExpandConstant('{app}\driver\UDM-development-public.cer') + '"', 'The UDM development certificate could not be installed.') then
      RaiseException('Certificate installation failed.');
    if not RunRequired(ExpandConstant('{sys}\rundll32.exe'), 'setupapi.dll,InstallHinfSection DefaultInstall 132 "' + ExpandConstant('{app}\driver\UdmWfp.inf') + '"', 'The UDM WFP driver could not be installed.') then
      RaiseException('Driver installation failed.');
    WriteNativeMessagingManifests();
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  ResultCode: Integer;
begin
  if CurUninstallStep = usUninstall then begin
    Exec(ExpandConstant('{sys}\sc.exe'), 'stop UdmWfp', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
    Exec(ExpandConstant('{sys}\sc.exe'), 'delete UdmWfp', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
    Exec(ExpandConstant('{sys}\certutil.exe'), '-delstore root ' + DriverThumbprint, '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
    RegDeleteKeyIncludingSubkeys(HKLM, 'SOFTWARE\Google\Chrome\NativeMessagingHosts\' + HostName);
    RegDeleteKeyIncludingSubkeys(HKLM, 'SOFTWARE\Microsoft\Edge\NativeMessagingHosts\' + HostName);
    RegDeleteKeyIncludingSubkeys(HKLM, 'SOFTWARE\Chromium\NativeMessagingHosts\' + HostName);
    RegDeleteKeyIncludingSubkeys(HKLM, 'SOFTWARE\Mozilla\NativeMessagingHosts\' + HostName);
  end;
end;
