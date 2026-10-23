$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/titanstrike-rune-visuals"
New-Item -ItemType Directory -Force $out | Out-Null
function Extract($s,$start,$end,$name){$a=$s.IndexOf($start);$b=$s.IndexOf($end,$a);if($a -lt 0 -or $b -lt $a){throw "Missing native section $start"};[IO.File]::WriteAllText("$out/$name.inc",$s.Substring($a,$b-$a))}
$s=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
Extract $s 'int32 CreatureTemplate::GetRandomValidModelId() const' 'int32 CreatureTemplate::GetFirstValidModelId() const' 'RandomModel'
Extract $s 'void Creature::UpdateMovementFlags()' 'void Creature::UpdateStat()' 'Movement'
Extract $s 'CreatureMovementData::CreatureMovementData()' 'std::string CreatureMovementData::ToString() const' 'MovementCtor'
$s=Get-Content "$repo/src/server/game/Entities/Creature/CreatureData.h" -Raw
Extract $s 'enum class CreatureGroundMovementType' 'const uint32 MAX_CREATURE_SPELLS' 'MovementData'
$s=Get-Content "$repo/src/server/game/Globals/ObjectMgr.cpp" -Raw
Extract $s 'int32 ObjectMgr::ChooseDisplayId(' 'void ObjectMgr::ChooseCreatureFlags(' 'ChooseDisplay'
$migration=Get-Content "$repo/sql/updates/world/2026_10_23_08_titanstrike_rune_visual_anchors.sql" -Raw
$ids='382291,382292,382294,382232,382234,382235,382240,382241,382242,382243,382246,382247,382248,382249,382250,382251'
$snapshot="SELECT c.id,c.modelid,m.Ground,m.Flight,c.position_z FROM creature c JOIN creature_movement_override m ON m.SpawnId=c.guid WHERE c.guid IN($ids) ORDER BY c.guid;"
$sql=@"
CREATE TEMPORARY TABLE saved_creature LIKE creature;
INSERT INTO saved_creature SELECT * FROM creature WHERE id IN(106322,106323,106324,64367);
CREATE TEMPORARY TABLE creature LIKE saved_creature;
INSERT INTO creature SELECT * FROM saved_creature;
UPDATE creature SET modelid=0 WHERE guid IN(382291,382292,382294);
CREATE TEMPORARY TABLE saved_movement LIKE creature_movement_override;
INSERT INTO saved_movement SELECT * FROM creature_movement_override;
CREATE TEMPORARY TABLE creature_movement_override LIKE saved_movement;
INSERT INTO creature_movement_override SELECT * FROM saved_movement;
DELETE FROM creature_movement_override WHERE SpawnId IN($ids);
INSERT INTO creature_movement_override(SpawnId,Ground,Flight,Rooted,InteractionPauseTimer) VALUES(382291,1,2,1,1234);
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceEntry=64367;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
UPDATE conditions SET ConditionValue2=20 WHERE SourceTypeOrReferenceId=22 AND SourceEntry=64367 AND SourceGroup=1 AND ConditionTypeOrReference=29;
$migration
SELECT 'FIRST';
$snapshot
$migration
SELECT 'SECOND';
$snapshot
SELECT 'END';
SELECT 'positions',COUNT(*) FROM creature a JOIN saved_creature b USING(guid) WHERE NOT(a.position_x=b.position_x AND a.position_y=b.position_y AND a.position_z=b.position_z AND a.orientation=b.orientation AND a.AiID=b.AiID AND a.PhaseId=b.PhaseId);
SELECT 'other-models',COUNT(*) FROM creature a JOIN saved_creature b USING(guid) WHERE a.guid NOT IN(382291,382292,382294) AND a.modelid<>b.modelid;
SELECT 'other-overrides',COUNT(*) FROM creature_movement_override WHERE SpawnId NOT IN($ids) AND SpawnId NOT IN(SELECT SpawnId FROM saved_movement);
SELECT 'extra-fields',Rooted,InteractionPauseTimer FROM creature_movement_override WHERE SpawnId=382291;
SELECT 'models',COUNT(*) FROM creature_model_info WHERE DisplayID IN(69560,69561,69562) AND DisplayID_Other_Gender=0;
SELECT 'radius',ConditionValue2 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=64367 AND SourceGroup=1 AND ConditionTypeOrReference=29;
SELECT 'stage',ConditionTarget,ConditionValue1,ConditionValue2 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=64367 AND SourceGroup=1 AND ConditionTypeOrReference=44;
SELECT 'recovered-height',COUNT(*) FROM creature WHERE id=64367 AND guid IN($ids) AND SQRT(POW(position_x-4880.67,2)+POW(position_y-317.193,2)+POW(position_z+37.238,2))>20 AND SQRT(POW(position_x-4880.67,2)+POW(position_y-317.193,2)+POW(position_z+37.238,2))<30;
UPDATE creature SET modelid=99999 WHERE guid=382291;
$migration
SELECT 'custom-model',modelid FROM creature WHERE guid=382291;
UPDATE creature SET map=0,modelid=0 WHERE guid=382291;
DELETE FROM creature_movement_override WHERE SpawnId=382291;
$migration
SELECT 'other-map',modelid FROM creature WHERE guid=382291;
SELECT 'other-map-override',COUNT(*) FROM creature_movement_override WHERE SpawnId=382291;
UPDATE conditions SET ConditionValue2=99 WHERE SourceTypeOrReferenceId=22 AND SourceEntry=64367 AND SourceGroup=1 AND ConditionTypeOrReference=44;
UPDATE conditions SET ConditionValue2=20 WHERE SourceTypeOrReferenceId=22 AND SourceEntry=64367 AND SourceGroup=1 AND ConditionTypeOrReference=29;
$migration
SELECT 'custom-gate',ConditionValue2 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=64367 AND SourceGroup=1 AND ConditionTypeOrReference=29;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Rune visual temporary SQL fixture failed'}}finally{$env:MYSQL_PWD=$old}
$a=[Array]::IndexOf($rows,'FIRST');$b=[Array]::IndexOf($rows,'SECOND');$d=[Array]::IndexOf($rows,'END')
$first=@($rows[($a+1)..($b-1)]);$second=@($rows[($b+1)..($d-1)])
if($first.Count -ne 16 -or ($first -join '|') -ne ($second -join '|')){throw 'Rune visual actor count/idempotence failed'}
foreach($expected in @("positions`t0","other-models`t0","other-overrides`t0","extra-fields`t1`t1234","models`t3","radius`t30","stage`t1`t1068`t2","recovered-height`t5","custom-model`t99999","other-map`t0","other-map-override`t0","custom-gate`t20")){if($rows -notcontains $expected){throw "Missing visual result $expected"}}
[IO.File]::WriteAllLines("$out/actors.tsv",[string[]]$first)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/TitanstrikeRuneVisualsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/actors.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native rune model/movement regression failed'}
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TitanstrikeRuneVisualCatalog.cpp -ProbeName TitanstrikeRuneVisualCatalog -OutputFile "$out/native.tsv"
$native=Get-Content "$out/native.tsv"
foreach($expected in @("DISPLAY`t69560`t8972","DISPLAY`t69561`t8973","DISPLAY`t69562`t8974","KIT`t10586","VISUAL`t191160`t49471")){if($native -notcontains $expected){throw "Native rune catalog mismatch $expected"}}
'PASS: native rune models/animation/lightning visual,16 scoped movement overrides, restored high-anchor admission and repeated SQL. Actual rendering and wave-by-wave visual cleanup remain client checks.'
