param([Parameter(Mandatory=$true)][string]$Catalog)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$PSScriptRoot/../../build-extractors/runtime-test/monk-acquisition-catalog-check" -Test {
 $rows=Get-Content $Catalog -Encoding UTF8
 foreach($line in @("ITEM`t128938`t1`t1`t-1","ITEM`t128940`t1`t1`t-1","CHILD`t128940`t133948`t16",
  "EFFECT`t199499`t0`t0`t249`t128940`t0`t1`t0","EFFECT`t217813`t0`t0`t249`t128938`t0`t1`t0",
  "EFFECT`t199502`t0`t0`t6`t1160`t0`t1`t0","EFFECT`t217814`t0`t2`t6`t1342`t0`t1`t0",
  "CRITERION`t983`t5`t92`t47821`t0","CRITERION`t983`t6`t92`t47977`t0",
  "CRITERION`t1137`t6`t92`t52482`t0","CRITERION`t1137`t7`t92`t52483`t0","CRITERION`t1137`t8`t92`t52751`t0",
  "DURATION`t199502`t9000","DURATION`t217814`t9000","SCENE_PACKAGE`t1524","SCENE_PACKAGE`t1668")){
  if($rows -notcontains $line){throw "Native26972 Monk pickup metadata differs: $line"}
 }
 'PASS typed26972 Monk native existing-item equip/scenes/pair/9000ms and both boss-pickup-next criteria sequences; actual scene/equip/client traversal still separate'
}
