// Shared production code; the fixture redirects only registry roots and injects a failed write.
const
  HostName = 'com.udm.download_manager';
  ChromiumId = 'kahfappnpjdcboccpnhinkcobcdgbdpl';
type
  TNativeRegistration = record
    Root: Integer;
    Key, Previous, Desired: String;
    Existed, Changed: Boolean;
  end;
  TNativeRegistrations = array[0..11] of TNativeRegistration;
#ifdef UDM_NATIVE_REGISTRATION_TEST
var
  NativeTestPrefix: String;
  NativeFailWriteAt, NativeWriteCount: Integer;
  NativeTestReplacementManifest, NativeTestReplacementContents: String;
#endif

#include "InstallerLock.iss"

function NativeMoveFileEx(ExistingName, NewName: String; Flags: Cardinal): Boolean;
  external 'MoveFileExW@kernel32.dll stdcall';

function NativeProductionRoot(Scope: Integer): Integer;
begin
  case Scope of
    0: Result := HKCU;
    1: Result := HKLM32;
    2: Result := HKLM64;
  else
    RaiseException('Unknown native-host registration scope.');
  end;
end;

function NativeRoot(Scope: Integer): Integer;
begin
#ifdef UDM_NATIVE_REGISTRATION_TEST
  Result := HKCU;
#else
  Result := NativeProductionRoot(Scope);
#endif
end;

function NativeKey(Scope, Browser: Integer): String;
begin
  case Browser of
    0: Result := 'SOFTWARE\Google\Chrome\NativeMessagingHosts\';
    1: Result := 'SOFTWARE\Microsoft\Edge\NativeMessagingHosts\';
    2: Result := 'SOFTWARE\Chromium\NativeMessagingHosts\';
    3: Result := 'SOFTWARE\Mozilla\NativeMessagingHosts\';
  else
    RaiseException('Unknown native-host browser.');
  end;
  Result := Result + HostName;
#ifdef UDM_NATIVE_REGISTRATION_TEST
  Result := NativeTestPrefix + '\Scope' + IntToStr(Scope) + '\' + Result;
#endif
end;

function NativeManifest(Directory: String; Browser: Integer): String;
begin
  if Browser = 3 then Result := AddBackslash(Directory) + 'firefox.json'
  else Result := AddBackslash(Directory) + 'chromium.json';
end;

function NativeJson(Value: String): String;
begin
  StringChangeEx(Value, '\', '\\', True);
  StringChangeEx(Value, '"', '\"', True);
  StringChangeEx(Value, #13, '\r', True);
  StringChangeEx(Value, #10, '\n', True);
  StringChangeEx(Value, #9, '\t', True);
  Result := Value;
end;

function NativeManifestContents(HostExecutable: String; Browser: Integer): String;
begin
  Result := '{"name":"' + HostName + '","description":"UDM browser download helper","path":"' + NativeJson(HostExecutable) + '","type":"stdio",';
  if Browser = 3 then Result := Result + '"allowed_extensions":["udm@local.example"]}'
  else Result := Result + '"allowed_origins":["chrome-extension://' + ChromiumId + '/"]}';
end;

function NativeCanRemoveManifest(FileName, ExpectedText: String): Boolean;
var Current: AnsiString;
begin
  if DirExists(FileName) then RaiseException('A directory replaced the native-host manifest; browser registration was preserved.');
  Result := not FileExists(FileName);
  if Result then Exit;
  if not LoadStringFromFile(FileName, Current) then RaiseException('Cannot inspect native-host manifest ownership; browser registration was preserved.');
  Result := Current = UTF8Encode(ExpectedText + #13#10);
end;

procedure NativePublishManifest(FileName, Contents: String);
var Lines: TArrayOfString; Temporary: String;
begin
  Temporary := FileName + '.new';
  SetArrayLength(Lines, 1); Lines[0] := Contents;
  try
    if not SaveStringsToUTF8FileWithoutBOM(Temporary, Lines, False) then
      RaiseException('Cannot write the native-host manifest.');
    if not NativeMoveFileEx(Temporary, FileName, 9) then
      RaiseException('Cannot publish the native-host manifest.');
  finally
    if FileExists(Temporary) then DeleteFile(Temporary);
  end;
end;

function NativeWriteRegistration(Root: Integer; Key, Value: String): Boolean;
begin
#ifdef UDM_NATIVE_REGISTRATION_TEST
  NativeWriteCount := NativeWriteCount + 1;
  if NativeWriteCount = NativeFailWriteAt then begin
    if NativeTestReplacementManifest <> '' then
      if not SaveStringToFile(NativeTestReplacementManifest, AnsiString(NativeTestReplacementContents), False) then RaiseException('Cannot inject replacement manifest.');
    Result := False; Exit;
  end;
#endif
  Result := RegWriteStringValue(Root, Key, '', Value);
end;

function NativeRestoreManifest(FileName, ExpectedText: String; Existed: Boolean; Before: AnsiString): Boolean;
var Current: AnsiString; Temporary: String;
begin
  Result := True;
  // Preserve a deleted or changed file: another owner may have replaced it.
  if not FileExists(FileName) then Exit;
  if not LoadStringFromFile(FileName, Current) then begin Result := False; Exit; end;
  if Current <> UTF8Encode(ExpectedText + #13#10) then Exit;
  if not Existed then begin Result := DeleteFile(FileName); Exit; end;
  Temporary := FileName + '.rollback';
  try
    Result := SaveStringToFile(Temporary, Before, False);
    if Result then Result := NativeMoveFileEx(Temporary, FileName, 9);
  finally
    if FileExists(Temporary) then DeleteFile(Temporary);
  end;
end;

procedure WriteNativeMessagingRegistrationUnlocked(Directory, HostExecutable: String);
var
  Entries: TNativeRegistrations;
  Scope, Browser, I, J: Integer;
  ChromiumManifest, FirefoxManifest, Current, Failure, ChromiumPublished, FirefoxPublished: String;
  ChromiumBefore, FirefoxBefore: AnsiString;
  ChromiumExisted, FirefoxExisted, RollbackOK, ChromiumChanged, FirefoxChanged: Boolean;
begin
  ChromiumManifest := NativeManifest(Directory, 0);
  FirefoxManifest := NativeManifest(Directory, 3);
  ChromiumChanged := False; FirefoxChanged := False;
  ChromiumExisted := FileExists(ChromiumManifest);
  FirefoxExisted := FileExists(FirefoxManifest);
  if ChromiumExisted and not LoadStringFromFile(ChromiumManifest, ChromiumBefore) then
    RaiseException('Cannot preserve the existing Chromium manifest.');
  if FirefoxExisted and not LoadStringFromFile(FirefoxManifest, FirefoxBefore) then
    RaiseException('Cannot preserve the existing Firefox manifest.');
  for Scope := 0 to 2 do for Browser := 0 to 3 do begin
    I := Scope * 4 + Browser;
    Entries[I].Root := NativeRoot(Scope);
    Entries[I].Key := NativeKey(Scope, Browser);
    Entries[I].Desired := NativeManifest(Directory, Browser);
    Entries[I].Changed := False;
    Entries[I].Existed := RegValueExists(Entries[I].Root, Entries[I].Key, '');
    if Entries[I].Existed and not RegQueryStringValue(Entries[I].Root, Entries[I].Key, '', Entries[I].Previous) then
      RaiseException('An existing native-host registration is not a readable string.');
  end;
  if not ForceDirectories(Directory) then RaiseException('Cannot create the native-host manifest directory.');
  try
    ChromiumPublished := NativeManifestContents(HostExecutable, 0);
    NativePublishManifest(ChromiumManifest, ChromiumPublished);
    ChromiumChanged := True;
    FirefoxPublished := NativeManifestContents(HostExecutable, 3);
    NativePublishManifest(FirefoxManifest, FirefoxPublished);
    FirefoxChanged := True;
    for I := 0 to 11 do begin
      if not NativeWriteRegistration(Entries[I].Root, Entries[I].Key, Entries[I].Desired) then
        RaiseException('Cannot register the UDM browser host.');
      Entries[I].Changed := True;
    end;
  except
    Failure := GetExceptionMessage;
    RollbackOK := True;
    for J := 11 downto 0 do if Entries[J].Changed then begin
      // A different installation that changed the value meanwhile keeps ownership.
      if RegQueryStringValue(Entries[J].Root, Entries[J].Key, '', Current) and
         (CompareText(Current, Entries[J].Desired) = 0) then begin
        if Entries[J].Existed then begin
          if not RegWriteStringValue(Entries[J].Root, Entries[J].Key, '', Entries[J].Previous) then RollbackOK := False;
        end else begin
          if not RegDeleteValue(Entries[J].Root, Entries[J].Key, '') then RollbackOK := False;
          RegDeleteKeyIfEmpty(Entries[J].Root, Entries[J].Key);
        end;
      end;
    end;
    if ChromiumChanged then
      if not NativeRestoreManifest(ChromiumManifest, ChromiumPublished, ChromiumExisted, ChromiumBefore) then RollbackOK := False;
    if FirefoxChanged then
      if not NativeRestoreManifest(FirefoxManifest, FirefoxPublished, FirefoxExisted, FirefoxBefore) then RollbackOK := False;
    if not RollbackOK then Failure := Failure + ' Previous registration could not be fully restored; see the setup log.';
    Log(Failure);
    RaiseException(Failure);
  end;
end;

procedure RemoveNativeMessagingRegistrationUnlocked(Directory, HostExecutable: String);
var Scope, Browser, Root: Integer; Key, Current, Expected, ChromiumText, FirefoxText: String;
  RemoveChromium, RemoveFirefox: Boolean;
begin
  ChromiumText := NativeManifestContents(HostExecutable, 0);
  FirefoxText := NativeManifestContents(HostExecutable, 3);
  // Preflight both files before changing any registration. A missing file permits
  // stale registration cleanup; a replacement keeps its registrations and bytes.
  RemoveChromium := NativeCanRemoveManifest(NativeManifest(Directory, 0), ChromiumText);
  RemoveFirefox := NativeCanRemoveManifest(NativeManifest(Directory, 3), FirefoxText);
  for Scope := 0 to 2 do for Browser := 0 to 3 do begin
    if ((Browser = 3) and RemoveFirefox) or ((Browser <> 3) and RemoveChromium) then begin
      Root := NativeRoot(Scope); Key := NativeKey(Scope, Browser);
      Expected := NativeManifest(Directory, Browser);
      if RegQueryStringValue(Root, Key, '', Current) and (CompareText(Current, Expected) = 0) then begin
        if not RegDeleteValue(Root, Key, '') then RaiseException('Cannot unregister the UDM browser host.');
        RegDeleteKeyIfEmpty(Root, Key);
      end;
    end;
  end;
  if RemoveChromium then
    if not NativeRestoreManifest(NativeManifest(Directory, 0), ChromiumText, False, '') then RaiseException('Cannot remove the owned Chromium manifest.');
  if RemoveFirefox then
    if not NativeRestoreManifest(NativeManifest(Directory, 3), FirefoxText, False, '') then RaiseException('Cannot remove the owned Firefox manifest.');
  RemoveDir(Directory);
end;

procedure WriteNativeMessagingRegistration(Directory, HostExecutable: String);
var Handle: THandle;
begin
 Handle:=NativeAcquireTransaction();
 try WriteNativeMessagingRegistrationUnlocked(Directory,HostExecutable);
 finally NativeReleaseTransaction(Handle);end;
end;
procedure RemoveNativeMessagingRegistration(Directory, HostExecutable: String);
var Handle: THandle;
begin
 Handle:=NativeAcquireTransaction();
 try RemoveNativeMessagingRegistrationUnlocked(Directory,HostExecutable);
 finally NativeReleaseTransaction(Handle);end;
end;