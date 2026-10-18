$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/grey-shoals"
New-Item -ItemType Directory -Force $out | Out-Null
function Query([string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Grey Shoals SQL check failed'}}finally{$env:MYSQL_PWD=$previous}
 $rows
}
$objectives=@(Query 'SELECT ID,Type,StorageIndex,ObjectID,Amount,Flags,TaskStep FROM quest_objectives WHERE QuestID=44737 ORDER BY ID;')
$expected=@("287220`t15`t-1`t0`t1`t0`t0","287221`t0`t1`t115025`t17`t92`t6","287222`t0`t2`t115031`t20`t92`t5","287223`t0`t3`t115026`t50`t92`t2","287226`t0`t0`t115739`t20`t92`t5","287238`t0`t4`t115751`t4`t92`t25")
if(($objectives -join ',') -ne ($expected -join ',')){throw 'Server objective mapping/weights differ from the native behavioral fixture'}
$objectives | Set-Content "$out/objectives.tsv"
$spawns=@(Query 'SELECT id,COUNT(*) FROM creature WHERE id IN(115025,115026,115031,115751) AND map=1220 AND areaId=8297 GROUP BY id ORDER BY id;')
if(($spawns -join ',') -ne "115025`t7,115026`t5,115031`t4,115751`t4"){throw 'Encounter spawn inventory changed; re-audit geometry and retail placement'}
if(@(Query 'SELECT guid FROM creature WHERE id IN(115025,115026,115031,115751) AND (map<>1220 OR areaId<>8297 OR modelid<>0);').Count){throw 'Unexpected actor placement/model override'}
Query 'SELECT guid,id,position_x,position_y,position_z,phaseMask,PhaseId,spawntimesecs FROM creature WHERE id IN(115025,115026,115031,115751) ORDER BY guid;' | Set-Content "$out/spawns.tsv"
$sai=@(Query "SELECT entryorguid,id FROM smart_scripts WHERE entryorguid IN(115025,115026,115031,115751) AND source_type=0 AND (Difficulties<>'' OR (event_flags & 30)<>0 OR event_param1>event_param2 OR event_param3>event_param4);")
if($sai.Count){throw 'Combat timers/difficulty admission changed'}
Query 'SELECT entryorguid,id,action_param1,target_type FROM smart_scripts WHERE entryorguid IN(115025,115026,115031,115751) AND source_type=0 ORDER BY entryorguid,id;' | Set-Content "$out/smart-spells.tsv"
$supplies=@(Query 'SELECT guid,id FROM gameobject WHERE guid BETWEEN 147040012 AND 147040015 AND map=1220 AND areaId=8297 ORDER BY guid;')
if(($supplies -join ',') -ne "147040012`t241741,147040013`t244704,147040014`t241741,147040015`t244704"){throw 'Reconstructed supply inventory changed; re-audit original-template restoration'}
$supplies | Set-Content "$out/supplies.tsv"
Query 'SELECT entry,name,displayId,Data10,VerifiedBuild FROM gameobject_template WHERE entry IN(241741,244704,266170);' | Set-Content "$out/supply-evidence.tsv"
if(@(Query "SELECT entry FROM creature_template WHERE entry IN(115025,115026,115031,115751) AND AIName<>'SmartAI';").Count){throw 'Combat AI binding changed'}
'PASS: live objective weights, 20 actor spawns, no model overrides, combat timer/difficulty data, four reconstructed supply objects. Spawn authenticity/rotation and full native casts are separate audit findings.'
