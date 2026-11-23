param(
 [string]$LabRoot='D:\UDM-Lab',
 [string]$CredentialPath=(Join-Path $env:LOCALAPPDATA 'UDM-Development\hyperv-lab-credential.clixml'),
 [int]$ImageIndex=1
)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath($LabRoot)
$owner=Get-Content -LiteralPath (Join-Path $root 'provisioning.json') -Raw | ConvertFrom-Json
$vm=Get-VM -Id $owner.vmId -ErrorAction Stop
if($vm.State -ne 'Off' -or $ImageIndex -lt 1){throw 'Prepare only a powered-off owned lab with a positive image index.'}
$folder=Join-Path $root 'bootstrap'
if(Test-Path -LiteralPath $folder){throw 'Bootstrap already exists; inspect it before retrying.'}
if(Test-Path -LiteralPath $CredentialPath){throw 'Guest credentials already exist; do not replace them implicitly.'}
New-Item -ItemType Directory -Path $folder | Out-Null
$acl=New-Object Security.AccessControl.DirectorySecurity
$sid=[Security.Principal.WindowsIdentity]::GetCurrent().User
$acl.SetOwner($sid);$acl.SetAccessRuleProtection($true,$false)
foreach($principal in @($sid.Value,'S-1-5-18','S-1-5-32-544')){
 $rule=New-Object Security.AccessControl.FileSystemAccessRule((New-Object Security.Principal.SecurityIdentifier($principal)),'FullControl','ContainerInherit,ObjectInherit','None','Allow')
 $acl.AddAccessRule($rule)
}
$vmAccount=New-Object Security.Principal.NTAccount('NT VIRTUAL MACHINE',([string]$vm.Id))
$vmSid=$vmAccount.Translate([Security.Principal.SecurityIdentifier])
$acl.AddAccessRule((New-Object Security.AccessControl.FileSystemAccessRule($vmSid,'ReadAndExecute','ContainerInherit,ObjectInherit','None','Allow')))
Set-Acl -LiteralPath $folder -AclObject $acl
$sourceFolder=Join-Path $folder 'source'
New-Item -ItemType Directory -Path $sourceFolder | Out-Null
$random=New-Object byte[] 24
$rng=[Security.Cryptography.RandomNumberGenerator]::Create()
try {$rng.GetBytes($random)}finally{$rng.Dispose()}
$password='UdM!'+([BitConverter]::ToString($random)).Replace('-','')
$credential=New-Object Management.Automation.PSCredential('UDMWorker',(ConvertTo-SecureString $password -AsPlainText -Force))
$credential | Export-Clixml -LiteralPath $CredentialPath -Encoding UTF8
$secret=[Security.SecurityElement]::Escape($password)
$answer=@"
<?xml version="1.0" encoding="utf-8"?>
<unattend xmlns="urn:schemas-microsoft-com:unattend" xmlns:wcm="http://schemas.microsoft.com/WMIConfig/2002/State">
 <settings pass="windowsPE">
  <component name="Microsoft-Windows-International-Core-WinPE" processorArchitecture="amd64" publicKeyToken="31bf3856ad364e35" language="neutral" versionScope="nonSxS">
   <SetupUILanguage><UILanguage>en-US</UILanguage></SetupUILanguage><InputLocale>0409:00000409</InputLocale><SystemLocale>en-US</SystemLocale><UILanguage>en-US</UILanguage><UserLocale>en-US</UserLocale>
  </component>
  <component name="Microsoft-Windows-Setup" processorArchitecture="amd64" publicKeyToken="31bf3856ad364e35" language="neutral" versionScope="nonSxS">
   <DynamicUpdate><Enable>false</Enable></DynamicUpdate>
   <DiskConfiguration><Disk wcm:action="add"><DiskID>0</DiskID><WillWipeDisk>true</WillWipeDisk>
    <CreatePartitions><CreatePartition wcm:action="add"><Order>1</Order><Type>EFI</Type><Size>260</Size></CreatePartition><CreatePartition wcm:action="add"><Order>2</Order><Type>MSR</Type><Size>16</Size></CreatePartition><CreatePartition wcm:action="add"><Order>3</Order><Type>Primary</Type><Extend>true</Extend></CreatePartition></CreatePartitions>
    <ModifyPartitions><ModifyPartition wcm:action="add"><Order>1</Order><PartitionID>1</PartitionID><Format>FAT32</Format><Label>System</Label></ModifyPartition><ModifyPartition wcm:action="add"><Order>2</Order><PartitionID>3</PartitionID><Format>NTFS</Format><Label>Windows</Label><Letter>C</Letter></ModifyPartition></ModifyPartitions>
   </Disk><WillShowUI>OnError</WillShowUI></DiskConfiguration>
   <ImageInstall><OSImage><Compact>true</Compact><InstallFrom><MetaData wcm:action="add"><Key>/IMAGE/INDEX</Key><Value>$ImageIndex</Value></MetaData></InstallFrom><InstallTo><DiskID>0</DiskID><PartitionID>3</PartitionID></InstallTo><WillShowUI>OnError</WillShowUI></OSImage></ImageInstall>
   <UserData><AcceptEula>true</AcceptEula><FullName>UDM Lab</FullName><Organization>UDM</Organization></UserData>
  </component>
 </settings>
 <settings pass="specialize"><component name="Microsoft-Windows-Shell-Setup" processorArchitecture="amd64" publicKeyToken="31bf3856ad364e35" language="neutral" versionScope="nonSxS"><ComputerName>UDM-LAB</ComputerName><TimeZone>Pacific Standard Time</TimeZone></component></settings>
 <settings pass="oobeSystem">
  <component name="Microsoft-Windows-International-Core" processorArchitecture="amd64" publicKeyToken="31bf3856ad364e35" language="neutral" versionScope="nonSxS"><InputLocale>0409:00000409</InputLocale><SystemLocale>en-US</SystemLocale><UILanguage>en-US</UILanguage><UserLocale>en-US</UserLocale></component>
  <component name="Microsoft-Windows-Shell-Setup" processorArchitecture="amd64" publicKeyToken="31bf3856ad364e35" language="neutral" versionScope="nonSxS">
   <OOBE><HideEULAPage>true</HideEULAPage><HideOnlineAccountScreens>true</HideOnlineAccountScreens><HideWirelessSetupInOOBE>true</HideWirelessSetupInOOBE><ProtectYourPC>1</ProtectYourPC></OOBE>
   <UserAccounts><LocalAccounts><LocalAccount wcm:action="add"><Name>UDMWorker</Name><DisplayName>UDM Lab Administrator</DisplayName><Group>Administrators</Group><Password><Value>$secret</Value><PlainText>true</PlainText></Password></LocalAccount></LocalAccounts></UserAccounts>
   <AutoLogon><Enabled>true</Enabled><LogonCount>1</LogonCount><Username>UDMWorker</Username><Password><Value>$secret</Value><PlainText>true</PlainText></Password></AutoLogon>
   <FirstLogonCommands><SynchronousCommand wcm:action="add"><Order>1</Order><Description>End one-time lab sign-in</Description><CommandLine>powershell.exe -NoProfile -Command &quot;Set-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon' AutoAdminLogon '0'; Remove-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon' DefaultPassword -ErrorAction SilentlyContinue&quot;</CommandLine></SynchronousCommand></FirstLogonCommands>
  </component>
 </settings>
</unattend>
"@
[xml]$validation=$answer
if($validation.unattend.settings.Count -ne 3){throw 'Answer file validation failed.'}
[IO.File]::WriteAllText((Join-Path $sourceFolder 'Autounattend.xml'),$answer,(New-Object Text.UTF8Encoding($false)))
$password=$null;$secret=$null;$answer=$null
$iso=Join-Path $folder 'answer.iso'
& (Join-Path $PSScriptRoot 'create-data-iso.ps1') -SourceFolder $sourceFolder -OutputPath $iso | Out-Null
# The answer ISO contains temporary guest credentials. Keep it under the protected ACL.
[ordered]@{vmId=[string]$vm.Id;answerIso=$iso;credentialPath=$CredentialPath;guestUser='UDMWorker';imageIndex=$ImageIndex;compactOsRequested=$true;guestInstalled=$false;answerIsoContainsTemporaryCredentials=$true} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $root 'answer-preparation.json') -Encoding UTF8
[ordered]@{prepared=$true;guestUser='UDMWorker';compactOsRequested=$true;credentialProtectedForCurrentUser=$true;guestInstalled=$false} | ConvertTo-Json
