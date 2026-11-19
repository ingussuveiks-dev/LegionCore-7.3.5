# Both counterfactual modes require their corresponding pre-fix game.lib.
param([switch]$Baseline,[string]$Migration,[switch]$CompletionBaseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if($Baseline -and $CompletionBaseline){throw 'Select one pre-fix counterfactual'}
$out="$repo/build-extractors/runtime-test/legion-linked-area-credit/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force "$out/logs"|Out-Null
if(!$Migration){$Migration="$repo/sql/updates/world/2026_11_19_07_linked_area_credit.sql"}else{$Migration=(Resolve-Path -LiteralPath $Migration).Path}
$previous=$env:LEGION_DIALOGUE_CONFIG;$previousMigration=$env:LEGION_AREA_DIALOGUE_MIGRATION;$compiler=$env:CL;$link=$env:_LINK_
$inputs=@("$PSScriptRoot/LegionLinkedAreaCreditNative.cpp","$PSScriptRoot/LegionStoryDialogueNative.cpp","$PSScriptRoot/HarbaronFragmentNative.cpp","$PSScriptRoot/LegionAreaDialogueNative.cpp",$Migration,"$repo/src/server/game/AI/SmartScripts/SmartAI.cpp","$repo/src/server/game/AI/SmartScripts/SmartScript.cpp","$repo/src/server/game/Entities/EventObject/EventObject.cpp","$repo/src/server/game/Conditions/ConditionMgr.cpp","$repo/build-extractors/src/server/game/Release/game.lib","$repo/build-extractors/src/server/scripts/Release/scripts.lib")
$sealed=@(Get-FileHash -LiteralPath $inputs|Select-Object Path,Hash);$sealed|ConvertTo-Json|Set-Content "$out/inputs-before.json"
try{
 $config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
 $config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
 [IO.File]::WriteAllText("$out/worldserver.conf",$config)
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:LEGION_AREA_DIALOGUE_MIGRATION=$Migration
 $env:CL="$compiler /DNDEBUG /Zi /I`"$repo/src/server/scripts/Legion`" /I`"$repo/src/server/scripts/Legion/MawofSouls`"";$env:_LINK_="$link /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 if($Baseline){$env:CL+=' /DLINKED_AREA_BASELINE'}
 # Uses the original pre-fix game.lib with source13 gate data.
 if($CompletionBaseline){$env:CL+=' /DLINKED_AREA_COMPLETION_BASELINE'}
 Push-Location "$repo/build-extractors/bin/Release"
 try{& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/LegionLinkedAreaCreditNative.cpp -ProbeName LegionLinkedAreaCreditNative -OutputFile "$out/native.tsv" *> "$out/compiler.log"
  if(!$? -or $LASTEXITCODE){throw "Native area dialogue verification failed:$out/compiler.log"}
  Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|COUNTERFACTUAL|INTRO)'}
 }}finally{Pop-Location}
}finally{
 $env:LEGION_DIALOGUE_CONFIG=$previous;$env:LEGION_AREA_DIALOGUE_MIGRATION=$previousMigration;$env:CL=$compiler;$env:_LINK_=$link
 if(Test-Path -LiteralPath "$out/worldserver.conf"){Remove-Item -LiteralPath "$out/worldserver.conf" -Force}
 foreach($item in $sealed){if((Get-FileHash -LiteralPath $item.Path).Hash -ne $item.Hash){throw 'Area dialogue verification input changed'}}
}
"LINKED_AREA_OUTPUT $out"
