param([string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-kraliss-roar-data"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$previous=$env:LEGION_DIALOGUE_CONFIG
try{
 $env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf"
 & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SharasdalKralissRoarCatalog.cpp -ProbeName SharasdalKralissRoarCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"
 if(!$? -or $LASTEXITCODE){throw 'Native Kraliss roar catalog failed'}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$expected=@(
 "SPELL`t209748`tDeafening Roar","EFFECT`t209748`t0`t0`t2`t0`t0`t0`t22`t15","EFFECT`t209748`t0`t1`t68`t0`t0`t0`t22`t15",
 "SPELL`t209764`tWreck","EFFECT`t209764`t0`t0`t6`t4`t0`t0`t25`t0",
 "SPELL`t209755`tWreck","EFFECT`t209755`t0`t0`t3`t0`t0`t0`t22`t15",
 "SPELL`t209757`tWreck","EFFECT`t209757`t0`t0`t165`t0`t0`t0`t25`t0","EFFECT`t209757`t0`t1`t203`t0`t0`t209764`t25`t0","EFFECT`t209757`t0`t2`t6`t4`t0`t0`t25`t0",
 "SPELL`t20508`tCharge","EFFECT`t20508`t0`t0`t96`t0`t0`t0`t6`t0","EFFECT`t20508`t0`t1`t6`t4`t0`t0`t6`t0"
)
if(Compare-Object $expected @(Get-Content "$out/native.tsv")){throw 'Review changed matching native roar/Wreck/Charge effects or targets'}
'PASS native26972/read-onlyHotfix Deafening Roar209748 damage+interrupt, Wreck209764/209755/209757 chain and original20508 Charge targets retained. This verifies spell data, not actual client casts/timing/visual/audio/combat or full1066.'
