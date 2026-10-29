param([Parameter(Mandatory=$true)][string]$BeforeReport,[Parameter(Mandatory=$true)][string]$AfterReport,[Parameter(Mandatory=$true)][string]$NativeCatalog)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$repo/build-extractors/runtime-test/artifact-triggered-loot-audit" -Test {
 $before=Get-Content $BeforeReport -Raw -Encoding UTF8|ConvertFrom-Json
 $after=Get-Content $AfterReport -Raw -Encoding UTF8|ConvertFrom-Json
 foreach($report in @($before,$after)){
  if(@($report.routes).Count -ne 36 -or @($report.native_race_class_pairs).Count -ne 149){throw 'Full native artifact selection scope lost'}
  $pickup=@($report.routes|Where-Object {$_.item -eq 127829})[0].pickup_objects|Where-Object {$_.entry -eq 251878}
  if($pickup.native_item_trigger_paths.Count -ne 1 -or ($pickup.native_item_trigger_paths[0].path -join ',') -ne '189916,201012,201010' -or ($pickup.native_reachable_equip_spells -join ',') -ne '201010' -or $pickup.native_reachable_create_spells.Count){throw 'Actual Twinblades trigger/equip classification differs'}
  # The two original scene-only Monk paths remain visible as review items;
  # their absence of a direct equip effect must not imply a working grant.
  if((@($report.pickup_review_findings|Where-Object {$_.entry -in @(248086,251605)}|Select-Object -ExpandProperty entry|Sort-Object) -join ',') -ne '248086,251605'){throw 'Scene-only Monk pickup candidates lost or changed; review new inventory'}
 }
 $old=@($before.findings|Where-Object {$_.kind -eq 'missing_artifact_chest_loot_link' -and $_.entry -eq 251878})
 if($old.Count -ne 1 -or @($after.findings|Where-Object {$_.entry -eq 251878}).Count){throw 'Original missing-loot counterfactual is not paired with repaired audit'}
 $native=Get-Content $NativeCatalog -Encoding UTF8
 foreach($line in @("EFFECT`t189916`t0`t2`t64`t0`t201012`t0`t0","EFFECT`t201012`t0`t0`t6`t0`t201010`t1`t0","EFFECT`t201010`t0`t0`t249`t127829`t0`t1`t0","CHILD`t127829`t127830`t16")){
  if($native -notcontains $line){throw "Matching26972 typed catalog does not confirm audit path: $line"}
 }
 'PASS full36 audit: pinned old snapshot detects189916->201012->201010 missing loot; actual repaired link clears it; matching native26972 confirms existing-item equip/pair; two scene-only Monk grant candidates remain explicit review'
}
