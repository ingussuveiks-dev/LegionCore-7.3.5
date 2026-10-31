param([string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-acquisition-catalog"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$prior=$env:LEGION_DIALOGUE_CONFIG
try{
 $env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf"
 foreach($name in @('SharasdalPickupCatalog','SharasdalReturnCatalog','SharasdalPickupSurface')){& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile "$name.cpp" -ProbeName $name -OutputFile "$out/$name.tsv";if(!$?){throw "Native $name failed"}}
}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
$rows=Get-Content "$out/SharasdalPickupCatalog.tsv"
foreach($row in @(
 "EFFECT`t209683`t0`t0`t42`t125`t209684`t17`t0",
 "EFFECT`t209684`t0`t0`t6`t1255`t0`t1`t0","AURA`t209684`t0`t430`t0","DURATION`t209684`t15000",
 "EFFECT`t209684`t0`t2`t61`t49820`t0`t1`t0",
 "EFFECT`t209685`t0`t0`t61`t49820`t0`t1`t0","EFFECT`t209685`t0`t1`t249`t128911`t0`t1`t0",
 "EFFECT`t209685`t0`t2`t134`t105787`t0`t1`t0","EFFECT`t209685`t0`t3`t64`t0`t225883`t1`t0",
 "EFFECT`t209743`t0`t0`t219`t1923`t0`t18`t0",
 "ITEM`t128911`t1`t1`t-1`t21`t42","ITEM`t128934`t1`t1`t-1`t14`t0","CHILD`t128911`t128934`t16",
 "CRITERION`t1066`t5`t92`t49820`t1","STEP_REWARD`t1066`t5`t0","SCENE_PACKAGE`t1609","OUTRO_VISUAL_COUNT`t209743`t0"
)){if($rows -notcontains $row){throw "Native acquisition metadata changed: $row"}}
$scene=Get-Content "$out/SharasdalPickupCatalog.tsv" -Raw
foreach($token in @("SCENE_TEXT`t1609`t14075",'Smooth Phase Complete','PlayWeaponScene()','Wait(10)')){if(!$scene.Contains($token)){throw "Actual scene1609 closure changed: $token"}}
$returns=Get-Content "$out/SharasdalReturnCatalog.tsv"
foreach($row in @("EFFECT`t210252`t0`t0`t64`t0`t0`t210247`t1`t0","EFFECT`t210252`t0`t1`t252`t0`t1285`t0`t1`t17","EFFECT`t210252`t0`t2`t90`t0`t101673`t0`t1`t0","EFFECT`t210252`t0`t3`t90`t0`t105806`t0`t1`t0")){if($returns -notcontains $row){throw "Native return metadata changed: $row"}}
$surface=@(Get-Content "$out/SharasdalPickupSurface.tsv"|Where-Object {$_ -like "SURFACE`t*"});if($surface.Count -ne 25){throw 'Incomplete native VMAP footprint'}
foreach($row in $surface){$f=$row.Split("`t");$z=[double]::Parse($f[3],[Globalization.CultureInfo]::InvariantCulture);if($f[4] -ne '1' -or [Math]::Abs($z-805.731) -gt 0.001){throw 'Original artifact landing has no supporting native floor'}}
'PASS actual26972/read-onlyHotfix jump/scene/equip/event/eligible-group credit/child/outro/return metadata, complete scene1609 closure and25 real VMAP footprint samples. Native210247 dummy effects are recorded, not claimed to perform missing follower cleanup; actual client and entire return sequence remain separate.'
