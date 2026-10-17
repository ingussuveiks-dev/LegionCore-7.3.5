$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-mists"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Read-Mists([string]$database,[string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match "^${database}DatabaseInfo\s*="} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Mists data query failed'}}finally{$env:MYSQL_PWD=$previous}
 @($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")})
}
$sql=@'
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup,a.SpecialFlags,q.QuestPackageID FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=40044;
SELECT 'objective',QuestID,ID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID=40044 ORDER BY StorageIndex;
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest=40044;
SELECT 'ender',id,quest FROM creature_questender WHERE quest=40044;
SELECT 'go-relation',id,quest FROM gameobject_queststarter WHERE quest=40044 UNION ALL SELECT 'go-relation',id,quest FROM gameobject_questender WHERE quest=40044;
SELECT 'actor',t.entry,w.KillCredit1,w.KillCredit2,t.faction,t.npcflag,t.unit_flags,t.unit_flags2,t.AIName,t.ScriptName,w.Displayid1,t.flags_extra,t.VehicleId,w.Type,t.unit_flags3 FROM creature_template t JOIN creature_template_wdb w ON w.Entry=t.entry WHERE t.entry IN(98014,98015,98016,98018) ORDER BY t.entry;
SELECT 'spawn',guid,id,map,position_x,position_y,position_z,modelid,phaseMask,PhaseId,spawnMask,spawntimesecs,MovementType,unit_flags,npcflag,unit_flags3 FROM creature WHERE id IN(98014,98015,98016,98018,97973) ORDER BY id,guid;
SELECT 'scaling',Entry,LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry IN(98014,98015,98016,98018) ORDER BY Entry;
SELECT 'addon',entry,path_id,bytes1,auras FROM creature_template_addon WHERE entry IN(98014,98015,98016,98018) ORDER BY entry;
SELECT 'model',DisplayID,BoundingRadius,CombatReach FROM creature_model_info WHERE DisplayID IN(65144,70529,25668,23553) ORDER BY DisplayID;
SELECT 'spell',entry,spell,difficultyMask,Difficulties FROM creature_template_spell WHERE entry IN(98014,98015,98016,98018) ORDER BY entry,spell;
SELECT 'giver',entry,npcflag,faction,AIName,ScriptName FROM creature_template WHERE entry=97973;
SELECT 'crew',t.entry,w.Displayid1,t.faction,t.unit_flags,t.AIName,t.ScriptName FROM creature_template t JOIN creature_template_wdb w ON w.Entry=t.entry WHERE t.entry=110806;
SELECT 'other-conditions',COUNT(*) FROM conditions WHERE(SourceTypeOrReferenceId IN(19,20) AND SourceEntry=40044) OR(SourceTypeOrReferenceId=22 AND SourceEntry IN(98014,98015,98016,98018));
SELECT 'smart',COUNT(*) FROM smart_scripts WHERE source_type=0 AND(entryorguid IN(98014,98015,98016,98018) OR entryorguid IN(SELECT -CAST(guid AS SIGNED) FROM creature WHERE id IN(98014,98015,98016,98018)));
SELECT 'spawn-addon',COUNT(*) FROM creature_addon WHERE guid IN(SELECT guid FROM creature WHERE id IN(98014,98015,98016,98018));
'@
$rows=Read-Mists World $sql
$rows | Set-Content "$out/world.tsv"
foreach($r in @("quest`t40044`t33554440`t0`t18446744073709551615`t0`t0`t0`t0`t0`t0`t9687","starter`t97973`t40044","ender`t97973`t40044","giver`t97973`t3`t35`tSmartAI`t")){if($rows -notcontains $r){throw "Mists contract changed: $r"}}
$actors=@{98014=@(65144,280195,0,7);98015=@(70529,280196,1,5);98016=@(25668,280197,2,5);98018=@(23553,280198,3,4)}
foreach($id in $actors.Keys){$v=$actors[$id];foreach($r in @("actor`t$id`t0`t0`t2817`t0`t32768`t2048`t`t`t$($v[0])`t0`t0`t7`t0","objective`t40044`t$($v[1])`t0`t$id`t1`t$($v[2])`t0","scaling`t$id`t98`t110","addon`t$id`t0`t0`t")){if($rows -notcontains $r){throw "Mists actor contract changed: $r"}};if(@($rows | Where-Object {$_ -like "spell`t$id`t*"}).Count -ne $v[3]){throw "Mists spell list changed: $id"}}
foreach($kind in @('objective','actor','addon','model')){if(@($rows | Where-Object {$_ -like "$kind`t*"}).Count -ne 4){throw "Expected four $kind rows"}}
foreach($kind in @('starter','ender')){if(@($rows | Where-Object {$_ -like "$kind`t*"}).Count -ne 1){throw "Unexpected Mists $kind set"}}
if(@($rows | Where-Object {$_ -like "go-relation`t*"}).Count){throw 'Unexpected GO quest relation'}
if($rows -notcontains "crew`t110806`t70163`t2817`t32768`t`t"){throw 'Missing native Captain Call crew template'}
$spawns=@($rows | Where-Object {$_ -like "spawn`t*"});if($spawns.Count -ne 5){throw 'Expected four enemies and one Havi spawn'}
foreach($s in $spawns){$v=$s.Split("`t");if($v[3] -ne '1220' -or $v[7] -ne '0' -or $v[8] -ne '1' -or $v[9] -ne '' -or $v[10] -ne '1' -or $v[12] -ne '0' -or $v[13] -ne '0' -or $v[14] -ne '0' -or $v[15] -ne '0'){throw "Unexpected Mists spawn override: $s"}}
foreach($kind in @('other-conditions','smart','spawn-addon')){if($rows -notcontains "$kind`t0"){throw "Unexpected $kind"}}
foreach($s in @($rows | Where-Object {$_ -like "spell`t*"})){$v=$s.Split("`t");if($v[3] -ne '0' -or $v[4] -ne ''){throw 'Unexpected outdoor spell restriction'}}
$sql=@'
SELECT 'display',COUNT(*) FROM creature_display_info WHERE ID IN(65144,70529,25668,23553,70163); SELECT 'faction',COUNT(*) FROM faction_template WHERE ID IN(2817,35); SELECT 'spells',COUNT(*) FROM spell_effect WHERE SpellID IN(186553,213738,215377,220399,220428,220431,220570,220622,220636,220638,220585,220409,220436); SELECT 'misc',COUNT(*) FROM spell_misc WHERE SpellID IN(186553,213738,215377,220399,220428,220431,220570,220622,220636,220638,220585,220409,220436);
'@
$rows=Read-Mists Hotfix $sql;$rows | Set-Content "$out/hotfix.tsv"
foreach($kind in @('display','faction','spells','misc')){if($rows -notcontains "$kind`t0"){throw "Native Mists $kind overridden"}}
'PASS: four original hostile objectives, Havi starter/ender, four active scaling ranges, native models/poses,21 unrestricted creature spells; no extra quest/SmartAI/spawn override.'
