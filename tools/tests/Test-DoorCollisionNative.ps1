param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/door-collision-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force "$out/logs"|Out-Null
$path="$repo/build-extractors/bin/Release/worldserver.conf";$hash=(Get-FileHash $path).Hash;$config=Get-Content $path -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'));[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$previous=$env:LEGION_DIALOGUE_CONFIG;$compiler=$env:CL;$link=$env:_LINK_
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$compiler /DNDEBUG /Zi /I`"$repo/src/server/scripts/Legion`"";if($Baseline){$env:CL+=' /DDOOR_COLLISION_BASELINE'}
 $env:_LINK_="$link /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/DoorCollisionNative.cpp -ProbeName DoorCollisionNative -OutputFile "$out/native.tsv" *> "$out/compiler.log"
  if(!$? -or $LASTEXITCODE){throw "Native door collision failed: $out/compiler.log"}
 }}finally{Pop-Location}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$compiler;$env:_LINK_=$link}
if((Get-FileHash $path).Hash -ne $hash){throw 'Canonical config changed'}
if((Test-Path "$out/logs/DBErrors.log") -and (Get-Item "$out/logs/DBErrors.log").Length){throw 'Native collision DBErrors unresolved'}
Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|COUNTERFACTUAL)'}
