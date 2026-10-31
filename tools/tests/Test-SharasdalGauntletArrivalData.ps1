param([string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-gauntlet-arrival-data"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$prior=$env:LEGION_DIALOGUE_CONFIG
try{
 $env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf"
 foreach($name in @('SharasdalGauntletCatalog','SharasdalHealCatalog')){& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile "$name.cpp" -ProbeName $name -OutputFile "$out/$name.tsv";if(!$? -or $LASTEXITCODE){throw "Native $name failed"}}
}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
$rows=Get-Content "$out/SharasdalGauntletCatalog.tsv"
foreach($row in @("EFFECT`t208580`t0`t0`t6`t430`t1247`t0`t25`t0","EFFECT`t209992`t0`t0`t252`t0`t0`t0`t1`t17","EFFECT`t209992`t0`t1`t164`t0`t0`t208580`t1`t0","EFFECT`t209992`t0`t2`t61`t0`t50336`t0`t1`t0","EFFECT`t209132`t0`t0`t6`t23`t0`t209184`t1`t0","EFFECT`t209132`t0`t1`t6`t395`t6547`t0`t1`t0","EOF`tgauntlet-native")){if($rows -notcontains $row){throw "Matching native gauntlet payload changed: $row"}}
if(@($rows|Where-Object {$_ -match '^EFFECT\t\d+\t\d+\t\d+\t61\t\d+\t50314\t'}).Count){throw 'Review a new native50314 spell producer'}
$criteria=Get-Content "$out/SharasdalHealCatalog.tsv"
foreach($row in @("STEP`t3`t2286`t47130`t0`tWater Gauntlet","TREE`t3`t47131`t47130`t0`t1`t29795`tUse Erunak's spell to ascend the riptide.","CRITERION`t3`t29795`t92`t50336`t1","TREE`t3`t47378`t47130`t0`t1`t29661`tRun through the wave gauntlet.","CRITERION`t3`t29661`t92`t50314`t1")){if($criteria -notcontains $row){throw "Native gauntlet criterion changed: $row"}}
$main=($rows -join "`n");$at=$main.IndexOf("SCENE_TEXT`t1604`t14066`t");if($at -lt 0){throw 'Actual elevator scene missing'};$main=$main.Substring($at)
foreach($token in @('Wait(2.0)','SetCameraFollowPath( genericFOV90, 5, 16781, 16782, 15)','Wait(15)','scene:SetCamera(0, 3, nil, nil)','Wait(3)','scene:EndScene()')){if(!$main.Contains($token)){throw "Actual1604/14066 elevator sequence changed: $token"}}
$query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'actor',c.guid,c.id,c.map,c.position_x,c.position_y,c.position_z,c.PhaseID,t.ScriptName FROM creature c JOIN creature_template t ON t.entry=c.id WHERE c.guid IN(362091,362092,362093,362095) ORDER BY c.guid;
SELECT 'script-entry',entry,ScriptName FROM creature_template WHERE ScriptName='npc_lady_zitrin' ORDER BY entry;
SELECT 'scene',SceneScriptPackageID,MiscValue,PlaybackFlags,COALESCE(CustomDuration,0),CONCAT('[',COALESCE(ScriptName,''),']') FROM spell_scene WHERE MiscValue=1247;
SELECT 'click',npc_entry,spell_id,cast_flags,user_type,add_npc_flag FROM npc_spellclick_spells WHERE spell_id IN(208580,209992) ORDER BY npc_entry,spell_id;
SELECT 'negative-link',spell_trigger,spell_effect,type FROM spell_linked_spell WHERE ABS(spell_trigger) IN(208580,209992) OR ABS(spell_effect) IN(208580,209992);
SELECT 'destination',id,target_map,target_position_x,target_position_y,target_position_z,target_orientation FROM spell_target_position WHERE id=209992;
SELECT 'extra',COUNT(*) FROM achievement_criteria_data WHERE criteria_id IN(29661,29795);
SELECT 'other-producer',COUNT(*) FROM smart_scripts WHERE action_type=205 AND action_param1=92 AND action_param2 IN(50314,50336);
SELECT 'EOF','gauntlet-arrival-data';
'@
[IO.File]::WriteAllText("$out/read.sql",$query)
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$prior=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 $sqlArguments=@("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--batch','--raw','--skip-column-names')
 $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $sqlArguments -InputFile "$out/read.sql" -OutputFile "$out/world.tsv" -ErrorFile "$out/world.stderr"
 if($code -or (Get-Item "$out/world.stderr").Length){throw 'Gauntlet full-EOF read-only context failed'}
}finally{$env:MYSQL_PWD=$prior}
$expected=@'
actor	362091	104856	1600	-21.1371	802.283	807.444	6120	npc_lady_zitrin
actor	362092	105298	1600	-181.168	801.922	796.974	6120	npc_grash
actor	362093	102826	1600	-189.884	805.469	797.067	6120	npc_erunak_stonespeaker
actor	362095	104856	1600	50.8681	803.073	805.875	6120	npc_lady_zitrin
script-entry	104856	npc_lady_zitrin
scene	1604	1247	20	0	[]
click	38821	208580	3	0	1
negative-link	-208580	209992	0
destination	209992	1600	-187.08	802.57	796.5	0
extra	0
other-producer	0
EOF	gauntlet-arrival-data
'@
if(Compare-Object ($expected.Replace("`r",'') -split "`n") @(Get-Content "$out/world.tsv")){throw 'Review changed native actor/scene/elevator/ascent/source identity'}
'PASS actual26972/read-onlyHotfix native3/ascent47131/50336 and separate arrival47378/50314, full1604 scene closure/main14066, live native click38821/208580 and negative aura-removal209992 link/destination. No duplicate50314 spell/SAI producer or extra criterion data; no live mutation or actual client ascent/gauntlet claim.'
