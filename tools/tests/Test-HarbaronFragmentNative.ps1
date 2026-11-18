param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/harbaron-fragment-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force "$out/logs"|Out-Null
if($Baseline){$source=(& git -C $repo show 'c92852906:src/server/scripts/Legion/MawofSouls/boss_harbaron.cpp')-join "`n";[IO.File]::WriteAllText("$out/HarbaronBaseline.inc",$source)}
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'));$config+="`nLogger.spells=2,Server`n";[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$previous=$env:LEGION_DIALOGUE_CONFIG;$compiler=$env:CL;$link=$env:_LINK_
$inputs=@("$repo/src/server/scripts/Legion/MawofSouls/boss_harbaron.cpp","$repo/src/server/scripts/Legion/MawofSouls/instance_maw_of_souls.cpp","$repo/tools/tests/HarbaronFragmentNative.cpp","$repo/tools/tests/RockawayOffensiveNative.cpp","$repo/tools/tests/RockawayChannelNative.cpp","$repo/build-extractors/src/server/scripts/Release/scripts.lib","$repo/build-extractors/src/server/game/Release/game.lib","$repo/build-extractors/src/common/Release/common.lib","$repo/build-extractors/src/server/shared/Release/shared.lib")
$sealed=@(Get-FileHash -LiteralPath $inputs|Select-Object Path,Hash)
$sealed|ConvertTo-Json|Set-Content "$out/inputs-before.json"
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$compiler /DNDEBUG /Zi /I`"$out`" /I`"$repo/src/server/scripts/Legion`" /I`"$repo/src/server/scripts/Legion/MawofSouls`""
 if($Baseline){$env:CL+=' /DHARBARON_FRAGMENT_BASELINE'}
 $env:_LINK_="$link /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/HarbaronFragmentNative.cpp -ProbeName HarbaronFragmentNative -OutputFile "$out/native.tsv" *> "$out/compiler.log"
  if(!$? -or $LASTEXITCODE){throw "Native Harbaron verification failed: $out/compiler.log"}
  Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|COUNTERFACTUAL|STATE|CLEANUP)'}
 }}finally{Pop-Location}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$compiler;$env:_LINK_=$link;foreach($input in $sealed){if((Get-FileHash -LiteralPath $input.Path).Hash -ne $input.Hash){throw 'Native probe input changed while running'}}}
