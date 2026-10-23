$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-acquisition/survival-native.tsv"
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SurvivalArtifactCatalog.cpp -ProbeName SurvivalArtifactCatalog -OutputFile $out
$rows=Get-Content $out
$steps=@($rows | Where-Object {$_ -like "STEP`t973`t*"})
if($steps.Count -ne 13){throw 'Native Survival scenario does not have all thirteen stages'}
$orders=@($steps | ForEach-Object {[int]$_.Split("`t")[2]} | Sort-Object)
if(($orders -join ',') -ne ((0..12) -join ',')){throw 'Native Survival stage order changed'}
foreach($expected in @("QUEST`t220`t0`t41542","QUEST`t220`t1`t39427","QUEST`t220`t2`t40385","EFFECT`t197651`t0`t249`t0`t128808`t0`t0","EFFECT`t205589`t0`t6`t430`t1220`t0`t0","EFFECT`t205589`t1`t6`t26`t0`t0`t0","SCENE_TEXT`t13976`t7.0 Artifacts - Hunter - Surv - Loot Scene - Main")){
 if($rows -notcontains $expected){throw "Survival native catalog mismatch: $expected"}
}
foreach($expected in @("CRITERION`t4`t46187`t29350`t92`t48172`t3","CRITERION`t12`t46292`t29264`t92`t47689`t0")){
 if($rows -notcontains $expected){throw "Native Survival stage criterion mismatch: $expected"}
}
$final=@($steps | Where-Object {$_ -like "STEP`t973`t12`t1987`t46291`tTalonclaw`t*"})
if($final.Count -ne 1){throw 'Talonclaw native final-stage criterion changed'}
$text=$rows -join "`n"
foreach($expected in @('WEAPONID = 128808','CASTVISUALID = 53909','STATEVISUALID = 53909','INITSOUNDID = 77363','TEXTEFFECTANIM = 4586','SendTrigger("Smooth Phase Complete")','clone:EquipWeapon(WeaponSlot.MainHand, WEAPONID)')){
 if(!$text.Contains($expected)){throw "Native Talonclaw visual definition missing: $expected"}
}
'PASS: native Survival quest order41542/39427/40385, all13 scenario stages, actual scene package text and Talonclaw visual/equip definitions'
