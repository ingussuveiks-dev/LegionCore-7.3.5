param([switch]$VioletBaseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/legion-instance-progression-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force "$out/logs"|Out-Null
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'));$config+="`nLogger.spells=2,Server`n";[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$previous=$env:LEGION_DIALOGUE_CONFIG;$compiler=$env:CL;$link=$env:_LINK_
$inputs=@("$repo/src/server/scripts/Legion/MawofSouls/boss_harbaron.cpp","$repo/src/server/scripts/Legion/MawofSouls/instance_maw_of_souls.cpp","$repo/src/server/scripts/Legion/VioletHoldLegion/instance_violet_hold_legion.cpp","$repo/tools/tests/HarbaronFragmentNative.cpp","$repo/tools/tests/LegionInstanceProgressionNative.cpp","$repo/tools/tests/RockawayOffensiveNative.cpp","$repo/tools/tests/RockawayChannelNative.cpp","$repo/build-extractors/src/server/scripts/Release/scripts.lib","$repo/build-extractors/src/server/game/Release/game.lib","$repo/build-extractors/src/common/Release/common.lib","$repo/build-extractors/src/server/shared/Release/shared.lib")
$sealed=@(Get-FileHash -LiteralPath $inputs|Select-Object Path,Hash)
$sealed|ConvertTo-Json|Set-Content "$out/inputs-before.json"
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$compiler /DNDEBUG /Zi /I`"$out`" /I`"$repo/src/server/scripts/Legion`" /I`"$repo/src/server/scripts/Legion/MawofSouls`" /I`"$repo/src/server/scripts/Legion/VioletHoldLegion`""
 if($VioletBaseline){$env:CL+=' /DEXPECT_VIOLET_SAVE_BASELINE'}
 $violet=if($VioletBaseline){(& git -C $repo show 'ee492cf20:src/server/scripts/Legion/VioletHoldLegion/instance_violet_hold_legion.cpp') -join "`n"}else{Get-Content "$repo/src/server/scripts/Legion/VioletHoldLegion/instance_violet_hold_legion.cpp" -Raw}
 if(!$violet){throw 'Violet production source unavailable'}
 [IO.File]::WriteAllText("$out/VioletInstanceActual.inc",$violet)
 $env:_LINK_="$link /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 & 'C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' -X utf8 "$PSScriptRoot/prepare_legion_instance_doors.py" $out
 if($LASTEXITCODE){throw 'Actual native door extraction failed'}
 Push-Location "$repo/build-extractors/bin/Release"
 try{& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/LegionInstanceProgressionNative.cpp -ProbeName LegionInstanceProgressionNative -OutputFile "$out/native.tsv" *> "$out/compiler.log"
  if(!$? -or $LASTEXITCODE){throw "Native dungeon modes verification failed: $out/compiler.log"}
  Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|INSTANCE|COUNTERFACTUAL)'}
 }}finally{Pop-Location}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$compiler;$env:_LINK_=$link;foreach($input in $sealed){if((Get-FileHash -LiteralPath $input.Path).Hash -ne $input.Hash){throw 'Native probe input changed while running'}}}
