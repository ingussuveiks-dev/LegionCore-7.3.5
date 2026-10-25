param()
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/magnar-departure-visual-data"
New-Item -ItemType Directory -Force $out | Out-Null
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^HotfixDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previousPassword=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 $overlays=@('SELECT (SELECT COUNT(*) FROM spell_visual_event)+(SELECT COUNT(*) FROM spell_visual_kit_effect)+(SELECT COUNT(*) FROM spell_visual_kit_model_attach)+(SELECT COUNT(*) FROM spell_visual_effect_name);'|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE -or $overlays.Count -ne 1 -or $overlays[0] -ne '0'){throw 'Base-only visual parents have overlays; review them before interpreting the graph'}
}finally{$env:MYSQL_PWD=$previousPassword}
$previousConfig=$env:LEGION_DIALOGUE_CONFIG
try{
 $env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf"
 foreach($probe in @('MagnarHandoffCatalog','MagnarDepartureVisualCatalog')){
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile "$probe.cpp" -ProbeName $probe -OutputFile "$out/$probe.tsv" *> "$out/$probe.log"
  if(!$? -or $LASTEXITCODE){throw "Native $probe failed"}
 }
}finally{$env:LEGION_DIALOGUE_CONFIG=$previousConfig}
$native=Get-Content "$out/MagnarHandoffCatalog.tsv"
foreach($expected in @("TIME`t191883`t0`t5000`t5000","EFFECT`t191883`t0`t6`t23`t0`t0`t10`t191884`t1`t0","EFFECT`t191884`t0`t31`t0`t0`t0`t200`t0`t6`t0")){
 if($native -notcontains $expected){throw "Magnar periodic attack changed: $expected"}
}
foreach($expected in @("SPELL`t193515`tValarjar Ghost Visual","TIME`t193515`t0`t-1`t-1","EFFECT`t193515`t1`t6`t4`t0`t0`t0`t0`t1`t0","VISUAL`t193515`t50147`t0","TIME`t192085`t5000`t0`t0","EFFECT`t192085`t0`t42`t0`t0`t0`t216016`t0`t18`t0","EFFECT`t216016`t0`t252`t0`t1358`t0`t0`t0`t1`t17","EFFECT`t216016`t1`t16`t0`t39174`t0`t0`t0`t1`t0")){
 if($native -notcontains $expected){throw "Departure candidate effect changed: $expected"}
}
$visual=Get-Content "$out/MagnarDepartureVisualCatalog.tsv"
foreach($expected in @("SOURCE`troot-tables-base-plus-hotfix`tclient-parent-records-base-only","ACTOR`t64124`t2594`t122815`t35212`t2`t1","EVENT`t50147`t100945`t7`t8`t2`t59520`t0`t0`t0`t0","ATTACHMENT`t59520`t310837`t13651`t19`t0`t65535`t65535`t65535`t1`t166237`t0.4`t0","PROCEDURE`t59520`t8357`t22`t0`t0`t0.4`t1.55973e+07","EVENT`t57409`t122012`t1`t2`t1`t69899`t0`t0`t0`t0","EVENT`t57409`t122013`t3`t13`t1`t69900`t0`t0`t0`t0","ANIMATION`t69899`t27501`t65535`t115`t0","ANIMATION`t69900`t27502`t65535`t38`t0","ATTACHMENT`t69900`t326489`t8912`t34`t0`t65535`t65535`t65535`t1`t463430`t1`t0")){
 if($visual -notcontains $expected){throw "Departure candidate visual changed: $expected"}
}
if(@($visual|Where-Object {$_ -match '^EVENT\t'}).Count -ne 4){throw 'Departure candidate event graph changed'}
'PASS: native ghost193515 has infinite dummy aura/alpha0.4 attachment;192085/216016 contain real jump/teleport/39174 side effects. Their visual references are resolved, not rendered or identified as Magnar ascension. No production candidate spell was added.'
