param([string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-wall-data"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$saved=$env:LEGION_DIALOGUE_CONFIG
try{
 $env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf"
 & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SharasdalHealCatalog.cpp -ProbeName SharasdalHealCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"
 if(!$? -or $LASTEXITCODE){throw 'Matching native Sharasdal catalog failed'}
}finally{$env:LEGION_DIALOGUE_CONFIG=$saved}
$catalog=Get-Content "$out/native.tsv"
foreach($row in @("STEP`t2`t2284`t47126`t0`tSecure the Central Junction","TREE`t2`t47127`t47126`t0`t1`t29660`tAsk Grash to Smash the Ice Wall","CRITERION`t2`t29660`t92`t49817`t1","TREE`t2`t47330`t47126`t0`t1`t29774`tDefeat Kra'liss","CRITERION`t2`t29774`t92`t50240`t1")){if($catalog -notcontains $row){throw "Matching native wall/junction criterion changed: $row"}}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'wall',guid,id,map,position_x,position_y,position_z,orientation,state FROM gameobject WHERE id=248514 ORDER BY guid;
SELECT 'template',entry,type,displayId,CONCAT('[',ScriptName,']') FROM gameobject_template WHERE entry=248514;
SELECT 'model',DisplayID,BoundingRadius,CombatReach FROM creature_model_info WHERE DisplayID=69084;
SELECT 'grash-display',entry,DisplayId1,DisplayId2,DisplayId3,DisplayId4 FROM creature_template_wdb WHERE entry=105298;
SELECT 'grash-binding',entry,ScriptName FROM creature_template WHERE ScriptName='npc_grash' ORDER BY entry;
SELECT 'path',id,point,position_x,position_y,position_z,orientation,delay FROM waypoint_data_script WHERE id IN(10993808,10993809,10993810) ORDER BY id,point;
SELECT 'extra',COUNT(*) FROM achievement_criteria_data WHERE criteria_id=29660;
SELECT 'other-205',COUNT(*) FROM smart_scripts WHERE action_type=205 AND action_param1=92 AND action_param2=49817;
SELECT 'event-script',COUNT(*) FROM event_scripts WHERE id=49817;
SELECT 'EOF','sharasdal-wall-data';
'@
[IO.File]::WriteAllText("$out/read.sql",$query);$saved=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 $sqlArguments=@("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--batch','--raw','--skip-column-names')
 $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $sqlArguments -InputFile "$out/read.sql" -OutputFile "$out/world.tsv" -ErrorFile "$out/world.stderr"
 if($code -or (Get-Item "$out/world.stderr").Length){throw 'Wall full-EOF read-only context failed'}
}finally{$env:MYSQL_PWD=$saved}

$expected=@'
wall	134287	248514	1600	-345.885	808.552	257.732	4.65834	1
wall	134289	248514	1600	-310.972	840.566	256.671	6.2652	1
wall	134290	248514	1600	-272.254	807.823	257.198	4.60109	1
wall	134291	248514	1600	-515.168	806.866	253.713	4.60109	1
wall	134292	248514	1600	-310.519	775.826	258.868	3.03838	1
wall	134293	248514	1600	-212.795	638.913	285.043	4.68268	1
wall	134294	248514	1600	-164.214	801.259	796.48	4.6476	1
wall	134295	248514	1600	114.75	802.233	806.423	4.66255	1
wall	134298	248514	1600	-10.3385	802.349	806.072	4.65473	1
template	248514	0	8570	[]
model	69084	3.15883	4.5
grash-display	105298	69084	0	0	0
grash-binding	105298	npc_grash
path	10993808	1	-549.38	805.859	246.296	0	0
path	10993808	2	-538.993	806.847	250.064	0	0
path	10993808	3	-527.257	806.61	256.553	0	0
path	10993808	4	-518.386	806.675	257.344	0	3000
path	10993808	5	-505.672	806.344	257.442	0	0
path	10993808	6	-484.676	805.924	254.118	0	0
path	10993808	7	-477.677	805.785	252.829	0	0
path	10993808	8	-452.119	804.504	248.697	0	0
path	10993808	9	-424.819	805.679	248.546	0	0
path	10993808	10	-409.523	806.617	248.338	0	0
path	10993808	11	-398.198	807.213	248.05	0	0
path	10993808	12	-389.967	808.045	247.996	0	0
path	10993808	13	-379.837	809.229	248.205	0	0
path	10993809	1	-546.191	808.367	246.469	0	0
path	10993809	2	-538.53	808.819	250.396	0	0
path	10993809	3	-522.592	811.921	256.877	0	3000
path	10993809	4	-516.129	810.098	257.357	0	0
path	10993809	5	-510.786	808.612	257.491	0	3000
path	10993809	6	-475.177	799.494	252.397	0	0
path	10993809	7	-434.705	802.75	248.609	0	0
path	10993809	8	-421.783	803.834	248.556	0	0
path	10993809	9	-402.62	806.061	248.177	0	3000
path	10993809	10	-374.883	807.224	250.55	0	0
path	10993809	11	-361.839	807.043	257.726	0	0
path	10993810	1	-353.54	807.974	258.951	0	3000
path	10993810	2	-315.37	809.207	261.249	0	0
path	10993810	3	-295.28	809.113	260.844	0	0
path	10993810	4	-279.015	809.036	259.377	0	3000
path	10993810	6	-203.135	808.68	261.464	0	0
path	10993810	7	-205.608	808.472	261.755	0	0
extra	0
other-205	0
event-script	0
EOF	sharasdal-wall-data
'@
if(Compare-Object ($expected.Replace("`r",'') -split "`n") @(Get-Content "$out/world.tsv")){throw 'Review changed native wall identities/poses/three path vectors or duplicate criterion producer'}
$worldRows=@(Get-Content "$out/world.tsv"|ForEach-Object {,($_ -split "`t")})
foreach($wallPose in @(@('10993808','4','134291'),@('10993810','1','134287'),@('10993810','4','134290'))){
 $pointRow=@($worldRows|Where-Object {$_[0] -eq 'path' -and $_[1] -eq $wallPose[0] -and $_[2] -eq $wallPose[1]})
 $wallRow=@($worldRows|Where-Object {$_[0] -eq 'wall' -and $_[1] -eq $wallPose[2]})
 if($pointRow.Count -ne 1 -or $wallRow.Count -ne 1){throw 'Unique wall/point pose absent'}
 $square=0.0
 for($axis=0;$axis -lt 3;++$axis){
  $delta=[double]::Parse($pointRow[0][3+$axis],[cultureinfo]::InvariantCulture)-[double]::Parse($wallRow[0][4+$axis],[cultureinfo]::InvariantCulture)
  $square+=$delta*$delta
 }
 if([math]::Sqrt($square) -gt 9.5){throw 'Wall lies outside original range5 plus configured Grash reach4.5 even before native GO size'}
}
'PASS fresh native26972/Hotfix junction AND: wall49817 separate from Kraliss50240; actual nine wall spawns and three full paths in database point order, door type/model and no extra World205/event producer; full EOF/stderr inspected. Callback zero-based index is certified against native movement source separately; no SQL/character write/client claim'
