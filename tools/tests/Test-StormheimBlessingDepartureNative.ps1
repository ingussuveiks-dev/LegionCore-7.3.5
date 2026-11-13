param([switch]$Baseline)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$mode=if($Baseline){'baseline'}else{'fixed'}
$out="$repo/.codex/stormheim-departure-2026-11-13/$mode-$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
[void][IO.Directory]::CreateDirectory("$out/logs")
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$path='src/server/scripts/Legion/stormheim_blessing.cpp'
$source=if($Baseline){(& git -C $repo show "29884c644:$path") -join "`n"}else{Get-Content "$repo/$path" -Raw}
if(!$source){throw 'Complete production script unavailable'}
[IO.File]::WriteAllText("$out/BlessingProduction.inc",$source)
$previous=$env:CL;$oldConfig=$env:LEGION_DIALOGUE_CONFIG
try {
 $env:CL="$previous /DNDEBUG /I`"$repo/src/server/scripts/Legion`" /I`"$out`""
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf"
 & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$PSScriptRoot/Prepare-PetBattleOwnedNative.ps1" -Out $out
  $query=Get-Content "$out/NativeAltheaQuest.inc" -Raw
  [IO.File]::WriteAllText("$out/NativeAltheaQuest.inc",$query.Replace('40710,42504,42627,40043,39191','40710,42504,42627,40043,39191,39597'))
  $failure=$null
  try {& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile '../tests/StormheimBlessingDepartureNative.cpp' -ProbeName "StormheimBlessingDeparture$mode" -OutputFile "$out/native.tsv" *> "$out/build.log"}catch{$failure=$_}
  if($Baseline){
   if(!$failure -or !$failure.Exception.Message.Contains('failed with exit 1') -or !(Get-Content "$out/build.log" -Raw).Contains('Map departure must stop the actual ceremony and remove its root before detaching the owner')){Get-Content "$out/build.log" -Tail 12;throw 'Unexpected baseline failure'}
   'PASS expected-broken complete29884c644 script: real departure retains root;exit1;paired repaired regression required'
  }else{
   if($failure){Get-Content "$out/build.log" -Tail 15;throw $failure}
   if(!(Get-Content "$out/native.tsv" -Raw).Contains('PASS actual native Stormheim departure cases=10')){throw 'Native departure cases incomplete'}
   Get-Content "$out/native.tsv" -Tail 1
  }
 }
}finally{$env:CL=$previous;$env:LEGION_DIALOGUE_CONFIG=$oldConfig}
$global:LASTEXITCODE=0
