#ifndef AppVersion
  #define AppVersion "0.57.0"
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
Filename: "{app}\UDM.exe"; Description: "Launch UDM Download Manager"; Flags: nowait postinstall skipifsilent

[Code]
const
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
    if not RunRequired(ExpandConstant('{app}\network\Udm.Network.exe'), '--status', 'The signed network runtime failed verification.') then
      RaiseException('Network runtime verification failed.');
    WriteNativeMessagingManifests();
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then begin
    RegDeleteKeyIncludingSubkeys(HKLM, 'SOFTWARE\Google\Chrome\NativeMessagingHosts\' + HostName);
    RegDeleteKeyIncludingSubkeys(HKLM, 'SOFTWARE\Microsoft\Edge\NativeMessagingHosts\' + HostName);
    RegDeleteKeyIncludingSubkeys(HKLM, 'SOFTWARE\Chromium\NativeMessagingHosts\' + HostName);
    RegDeleteKeyIncludingSubkeys(HKLM, 'SOFTWARE\Mozilla\NativeMessagingHosts\' + HostName);
  end;
end;
