param([Parameter(Mandatory=$true)][string]$EvidenceDirectory,[string[]]$RuntimeDirectories=@(),[Parameter(Mandatory=$true)][scriptblock]$Test)
$ErrorActionPreference='Stop'
. "$PSScriptRoot/ServerDiagnosticLines.ps1"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
New-Item -ItemType Directory -Force $EvidenceDirectory|Out-Null
function Snapshot-Crashes {
 $result=@{}
 foreach($directory in @("$repo/build-extractors/bin/Release/Crashes")+@(@($EvidenceDirectory)+$RuntimeDirectories|ForEach-Object {"$_/Crashes"})){
  foreach($file in @(Get-ChildItem -LiteralPath $directory -File -ErrorAction SilentlyContinue)){$result[$file.FullName]=[ordered]@{Hash=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash;Length=$file.Length;ModifiedUtc=$file.LastWriteTimeUtc.ToString('o')}}
 }
 return $result
}
function Read-Appended([string]$path,[long]$offset){
 if(!(Test-Path -LiteralPath $path)){return ''}
 $stream=[IO.File]::Open($path,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
 try{if($stream.Length -lt $offset){$offset=0};[void]$stream.Seek($offset,[IO.SeekOrigin]::Begin);$reader=New-Object IO.StreamReader($stream);try{$reader.ReadToEnd()}finally{$reader.Dispose()}}finally{$stream.Dispose()}
}
$beforeCrashes=Snapshot-Crashes
$beforeLogs=@{}
foreach($directory in @("$repo/build-extractors/bin/Release/logs")+@(@($EvidenceDirectory)+$RuntimeDirectories|ForEach-Object {"$_/logs"})){
 foreach($name in @('DBErrors.log','Server.log')){
  $path="$directory/$name";$file=Get-Item -LiteralPath $path -ErrorAction SilentlyContinue
  $beforeLogs[$path]=[ordered]@{Length=if($file){$file.Length}else{0};ModifiedUtc=if($file){$file.LastWriteTimeUtc.ToString('o')}else{$null}}
 }
}
[IO.File]::WriteAllText("$EvidenceDirectory/diagnostics-before.json",(@{Crashes=$beforeCrashes;Logs=$beforeLogs;Utc=[DateTime]::UtcNow.ToString('o')}|ConvertTo-Json -Depth 6))
$completed=$false
try{& $Test *>&1 | Tee-Object -FilePath "$EvidenceDirectory/check-output.log"; $completed=$true}
finally{
 $afterCrashes=Snapshot-Crashes
 $changed=@($afterCrashes.Keys|Where-Object {!$beforeCrashes.ContainsKey($_) -or $beforeCrashes[$_].Hash -cne $afterCrashes[$_].Hash})
 $newDb=@();$serverDiagnostics=@();$index=0
 foreach($path in $beforeLogs.Keys){
  $tail=Read-Appended $path $beforeLogs[$path].Length
  [IO.File]::WriteAllText("$EvidenceDirectory/appended-$index-$([IO.Path]::GetFileName($path))",$tail);++$index
  if($path.EndsWith('/DBErrors.log') -and $tail.Length){$newDb+=@{Path=$path;Text=$tail}}
  $serverDiagnostics+=@(Get-ServerDiagnosticLines $tail)
 }
 [IO.File]::WriteAllText("$EvidenceDirectory/diagnostics-after.json",(@{Completed=$completed;Crashes=$afterCrashes;ChangedCrashes=$changed;AppendedDBErrors=$newDb;ServerDiagnostics=$serverDiagnostics;Utc=[DateTime]::UtcNow.ToString('o');Servers=@(Get-Process worldserver,bnetserver -ErrorAction SilentlyContinue|Select-Object Id,ProcessName,StartTime)}|ConvertTo-Json -Depth 6))
 if($changed.Count -or $newDb.Count -or $serverDiagnostics.Count){throw "New runtime diagnostics require investigation: $EvidenceDirectory/diagnostics-after.json"}
 "PASS post-test diagnostics: no new/changed crashes, appended DBErrors or server warnings/errors/assertions; test completion=$completed"
}
