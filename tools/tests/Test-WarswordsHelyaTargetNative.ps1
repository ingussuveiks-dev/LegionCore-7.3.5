param([string]$OutputDirectory='')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-helya-target-native"
if($OutputDirectory){$out=[IO.Path]::GetFullPath($OutputDirectory)}
New-Item -ItemType Directory -Force $out|Out-Null
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$originalHash=(Get-FileHash "$repo/build-extractors/bin/Release/worldserver.conf").Hash
$nativeInputs=@{}
foreach($inputPath in @("$repo/build-extractors/src/server/game/Release/game.lib","$repo/build-extractors/src/server/scripts/Release/scripts.lib","$PSScriptRoot/WarswordsHelyaTargetNative.cpp","$PSScriptRoot/RockawayChannelNative.cpp")){$nativeInputs[$inputPath]=(Get-FileHash -LiteralPath $inputPath).Hash}
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
New-Item -ItemType Directory -Force "$out/logs"|Out-Null
[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$previous=$env:LEGION_DIALOGUE_CONFIG
$previousCompiler=$env:CL
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf"
 $env:CL="$previousCompiler /DNDEBUG /I`"$repo/src/server/scripts/Legion`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{
  & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
   & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/WarswordsHelyaTargetNative.cpp -ProbeName WarswordsHelyaTargetNative -OutputFile "$out/native.tsv" *> "$out/catalog.log"
   if(!$? -or $LASTEXITCODE){throw 'Native Helya target execution failed'}
  }
 }finally{Pop-Location}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$previousCompiler}
if((Get-FileHash "$repo/build-extractors/bin/Release/worldserver.conf").Hash -ne $originalHash){throw 'Canonical configuration changed during native probe'}
foreach($inputPath in $nativeInputs.Keys){if((Get-FileHash -LiteralPath $inputPath).Hash -ne $nativeInputs[$inputPath]){throw 'Native Helya source/final-link inputs changed during execution'}}
$nativeInputs|ConvertTo-Json|Set-Content "$out/native-inputs.json"
if((Test-Path "$out/logs/DBErrors.log") -and (Get-Item "$out/logs/DBErrors.log").Length){throw 'Native probe DBErrors require review'}
Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(QUEUED|SELF|VIGFUS|PASS|METADATA)'}
