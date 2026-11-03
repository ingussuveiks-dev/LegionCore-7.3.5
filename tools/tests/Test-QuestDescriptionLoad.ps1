param([switch]$Counterfactual,[switch]$VerifyReleaseBinary,[ValidateSet('','Original','Repaired')][string]$MonkTextMode='',[string]$ReleaseDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/quest-description/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($repo,$out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $python='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
 & $python "$PSScriptRoot/export_quest_description_fixture.py" $out; if($LASTEXITCODE){throw 'Native quest description source export failed'}
 New-Item -ItemType Directory -Force "$out/logs"|Out-Null
 $canonical="$repo/build-extractors/bin/Release/worldserver.conf";$hash=(Get-FileHash $canonical).Hash
 $library="$repo/build-extractors/src/server/game/Release/game.lib";$libraryHash=(Get-FileHash $library).Hash
 $config=[regex]::Replace((Get-Content $canonical -Raw),'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
 [IO.File]::WriteAllText("$out/worldserver.conf",$config)
 $prior=$env:LEGION_DIALOGUE_CONFIG;$priorCompiler=$env:CL;$priorMode=$env:LEGION_QUEST_TEXT_COUNTERFACTUAL;$priorMonk=$env:LEGION_MONK_TEXT_EXPECTED
 try{
  $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$priorCompiler /I`"$out`"";$env:LEGION_QUEST_TEXT_COUNTERFACTUAL=if($Counterfactual){'1'}else{'0'}
  $env:LEGION_MONK_TEXT_EXPECTED=$MonkTextMode
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/QuestDescriptionLoadNative.cpp -ProbeName QuestDescriptionLoadNative -OutputFile "$out/native.tsv" *> "$out/native.log"
  if(!$? -or $LASTEXITCODE){throw "Native quest description regression failed: $out/native.log"}
  if((Get-FileHash $library).Hash -ne $libraryHash){throw 'Native input library changed during regression'}
  [IO.File]::WriteAllText("$out/library.json",(@{GameLibrarySHA256=$libraryHash;ExpectedCounterfactual=[bool]$Counterfactual;NativeExit=$LASTEXITCODE}|ConvertTo-Json))
  Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|EXPECTED COUNTERFACTUAL|MONK_TEXT)'}
  if((Get-Content "$out/native.tsv") -notcontains "LAYOUT`t24`t107`t472`t504`t536`t632`t664`t792`t824`t856`t888"){throw 'Actual native Field/Quest text layout differs from selected PE proof'}
 }finally{$env:LEGION_DIALOGUE_CONFIG=$prior;$env:CL=$priorCompiler;$env:LEGION_QUEST_TEXT_COUNTERFACTUAL=$priorMode;$env:LEGION_MONK_TEXT_EXPECTED=$priorMonk}
 if((Get-FileHash $canonical).Hash -ne $hash){throw 'Canonical config changed'}
 if($VerifyReleaseBinary){if($Counterfactual){throw 'Counterfactual cannot certify repaired Release'};if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$previous=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$previous";& $python "$PSScriptRoot/verify_quest_description_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Quest description Release check failed'}}finally{$env:PYTHONPATH=$previous}}
}
