# AdmissionBaseline measures the original pre-fix linked Player implementation.
# Use -Admission with the repaired game.lib; no switch inventories static tasks.
param([switch]$AdmissionBaseline,[switch]$Admission)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/legion-area-quest/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force "$out/logs"|Out-Null
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$previous=$env:LEGION_DIALOGUE_CONFIG;$compiler=$env:CL;$link=$env:_LINK_
$inputs=@("$repo/tools/tests/LegionAreaQuestNative.cpp","$repo/src/server/game/Entities/Player/Player.cpp","$repo/src/server/game/Globals/QuestData.cpp","$repo/src/server/game/Entities/EventObject/EventObject.cpp","$repo/build-extractors/src/server/game/Release/game.lib","$repo/build-extractors/src/server/scripts/Release/scripts.lib")
$sealed=@(Get-FileHash -LiteralPath $inputs|Select-Object Path,Hash);$sealed|ConvertTo-Json|Set-Content "$out/inputs-before.json"
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$compiler /DNDEBUG /Zi /I`"$repo/src/server/scripts/Legion`" /I`"$repo/src/server/scripts/Legion/MawofSouls`"";$env:_LINK_="$link /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 if($AdmissionBaseline -and $Admission){throw 'Choose the pre-fix counterfactual or repaired admission check'}
 if($AdmissionBaseline){$env:CL+=' /DAREA_ADMISSION_BASELINE'}
 if($Admission){$env:CL+=' /DAREA_ADMISSION_REPAIRED'}
 Push-Location "$repo/build-extractors/bin/Release"
 try{& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/LegionAreaQuestNative.cpp -ProbeName LegionAreaQuestNative -OutputFile "$out/native.tsv" *> "$out/compiler.log"
  if(!$? -or $LASTEXITCODE){throw "Native area quest verification failed:$out/compiler.log"}
  Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|COUNTERFACTUAL|OBJECTIVE)'}
 }}finally{Pop-Location}
}finally{
 $env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$compiler;$env:_LINK_=$link
 if(Test-Path -LiteralPath "$out/worldserver.conf"){Remove-Item -LiteralPath "$out/worldserver.conf" -Force}
 foreach($item in $sealed){if((Get-FileHash -LiteralPath $item.Path).Hash -ne $item.Hash){throw 'Area quest verification input changed'}}
}
"AREA_QUEST_OUTPUT $out"
