param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/eagle-blessing-catalog"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories "$repo/build-extractors/runtime-test/legion-legendary-catalog" -Test {
 $old=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile EagleBlessingCatalog.cpp -ProbeName EagleBlessingCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$old}
 $native=Get-Content "$out/native.tsv"
 foreach($line in @("QUEST_LINE`t0`t41542","QUEST_LINE`t1`t39427","QUEST_LINE`t2`t40385","PHASE`t7088`t548","SUMMON`t61`t1`t2`t2`t0","SUMMON`t3292`t1`t0`t135704`t0","VEHICLE_SEAT`t4535`t0`t16201","EFFECT`t198941`t0`t0`t28`t0`t100967`t61`t0`t46`t0`t0","EFFECT`t218564`t0`t0`t6`t428`t0`t0`t198941`t1`t0`t0","EFFECT`t217343`t0`t1`t28`t0`t100990`t3292`t0`t46`t0`t0","EFFECT`t220619`t0`t0`t6`t438`t0`t7088`t0`t1`t0`t0","EFFECT`t220791`t0`t0`t90`t0`t110938`t0`t0`t1`t0`t0","EFFECT`t220791`t0`t1`t61`t0`t53251`t0`t0`t38`t0`t0","EFFECT`t220621`t0`t0`t232`t0`t7088`t0`t0`t1`t0`t0")){
  if($native -notcontains $line){throw "Matching26972 blessing asset differs: $line"}
 }
 foreach($id in @(119067,119071,119073)){if(!@($native|Where-Object {$_ -like "TEXT`t$id`t*"}).Count){throw "Missing native Degar speech $id"}}
 'PASS matching26972 READ-ONLY blessing evidence/questline/phase/controller/summons/vehicle seat/credit/cleanup/text identities. This is an asset regression, NOT a passing live39427 quest or restored flight.'
}
