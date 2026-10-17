param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-dreadwake"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Read-Data([string]$database,[string]$sql,[string]$name){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match "^${database}DatabaseInfo\s*="} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw "Dreadwake $database query failed"}}finally{$env:MYSQL_PWD=$previous}
 $rows=@($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$rows | Set-Content "$out/$name.tsv";return $rows
}
$migration=Get-Content "$repo/sql/updates/world/2026_10_17_14_stormheim_dreadwake.sql" -Raw
$sql=''
if(!$Live){$sql=@'
CREATE TEMPORARY TABLE saved_creature_template LIKE creature_template;
INSERT INTO saved_creature_template SELECT * FROM creature_template WHERE entry IN(93624,93603,95212);
UPDATE saved_creature_template SET ScriptName='' WHERE entry=93624;
CREATE TEMPORARY TABLE creature_template LIKE saved_creature_template;
INSERT INTO creature_template SELECT * FROM saved_creature_template;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE NOT(SourceTypeOrReferenceId=15 AND SourceGroup=18657 AND SourceEntry=0) AND (SourceGroup IN(18657,95212) OR SourceEntry IN(38872,38873,39154,189100,189176));
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
'@;$sql+=$migration+"`n"+$migration+"`n"}
$sql+=@'
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup,a.SpecialFlags,q.QuestPackageID FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(38872,38873,39153,39385,39154,38878) ORDER BY q.ID;
SELECT 'objective',QuestID,ID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID IN(38872,38873,39154) ORDER BY QuestID,StorageIndex;
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest IN(38872,38873,39385,39154,38878) ORDER BY quest,id;
SELECT 'ender',id,quest FROM creature_questender WHERE quest IN(38872,38873,39154) ORDER BY quest,id;
SELECT 'incoming',ID FROM quest_template_addon WHERE NextQuestID=39154 ORDER BY ID;
SELECT 'actor',t.entry,t.gossip_menu_id,t.npcflag,t.npcflag2,t.faction,t.unit_flags,t.unit_flags2,t.AIName,t.ScriptName,w.Displayid1,w.KillCredit1,w.KillCredit2 FROM creature_template t JOIN creature_template_wdb w USING(entry) WHERE t.entry IN(93624,93603,95212) ORDER BY t.entry;
SELECT 'mount',entry,Displayid1 FROM creature_template_wdb WHERE entry IN(87442,109680) ORDER BY entry;
SELECT 'spawn',guid,id,map,position_x,position_y,position_z,modelid,phaseMask,PhaseId FROM creature WHERE id IN(93624,93603,109680,95212) ORDER BY id,guid;
SELECT 'menu',MenuID,OptionID,OptionNpc,OptionBroadcastTextID,ActionMenuID FROM gossip_menu_option WHERE MenuID=18657;
SELECT 'condition',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18657 AND SourceEntry=0 ORDER BY ElseGroup;
SELECT 'click',npc_entry,spell_id,cast_flags,user_type,add_npc_flag FROM npc_spellclick_spells WHERE npc_entry=95212;
SELECT 'smart',entryorguid,id,link,event_type,event_chance,event_flags,event_param1,action_type,action_param1,target_type,target_param1,target_param2,Difficulties FROM smart_scripts WHERE entryorguid=95212 AND source_type=0;
SELECT 'cullen-smart',COUNT(*) FROM smart_scripts WHERE entryorguid=93624 AND source_type=0;
SELECT 'extra-conditions',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId IN(19,20) AND SourceEntry IN(38872,38873,39154)) OR (SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(189100,189176)) OR (SourceTypeOrReferenceId=18 AND SourceGroup=95212);
'@
if(!$Live){$sql+=@"
SELECT 'protected',COUNT(*) FROM creature_template a JOIN saved_creature_template b USING(entry) WHERE a.entry IN(93603,95212) AND a.ScriptName=b.ScriptName AND a.AIName=b.AIName AND a.npcflag=b.npcflag;
UPDATE creature_template SET ScriptName='' WHERE entry=93624;
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(15,18657,0,6,67);
$migration
SELECT 'custom-preserved',ScriptName='' FROM creature_template WHERE entry=93624;
SELECT 'custom-conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18657 AND SourceEntry=0;
"@}
$rows=Read-Data World $sql "world$(if($Live){'-live'})"
foreach($r in @("quest`t38872`t33554440`t0`t234881970`t0`t38873`t0`t38873`t0`t0`t0","quest`t38873`t33554440`t0`t234881970`t0`t39154`t38872`t39154`t0`t0`t0","objective`t38872`t279665`t0`t93624`t1`t0`t28","objective`t38873`t278644`t0`t95391`t4`t0`t0","incoming`t38873","incoming`t39385","starter`t93624`t38872","ender`t93603`t38872","starter`t93603`t38873","ender`t93603`t38873","starter`t93603`t39154","ender`t94227`t39154","menu`t18657`t0`t0`t97471`t0","condition`t0`t9`t0`t38872`t0`t0`t0","condition`t1`t28`t0`t38872`t0`t0`t0","mount`t87442`t60307","mount`t109680`t60307","click`t95212`t189100`t1`t0`t1","smart`t95212`t0`t0`t8`t100`t0`t189100`t11`t189176`t11`t95436`t200`t","cullen-smart`t0","extra-conditions`t0")){if($rows -notcontains $r){throw "Dreadwake contract changed: $r"}}
if(!($rows | Where-Object {$_ -like "actor`t93624`t18657`t3`t0`t68`t131458`t2048`t`tnpc_stormheim_dreadwake_cullen`t65132`t0`t0"})){throw 'Cullen binding or flags changed'}
if((@($rows | Where-Object {$_ -like "incoming`t*"}) -join ',') -ne "incoming`t38873,incoming`t39153,incoming`t39385"){throw '39154 predecessor set changed'}
if(!$Live){foreach($r in @("protected`t2","custom-preserved`t1","custom-conditions`t3")){if($rows -notcontains $r){throw 'Dreadwake migration damaged custom/protected data'}}}
$h=Read-Data Hotfix "SELECT 'effects',COUNT(*) FROM spell_effect WHERE SpellID IN(189100,189176); SELECT 'taxi',COUNT(*) FROM taxi_path WHERE ID=5262; SELECT 'taxinodes',COUNT(*) FROM taxi_nodes WHERE ID=1722; SELECT 'taxipoints',COUNT(*) FROM taxi_path_node WHERE PathID=5262;" 'hotfix'
foreach($r in @("effects`t0","taxi`t0","taxinodes`t0","taxipoints`t0")){if($h -notcontains $r){throw 'Unexpected native Dreadwake Hotfix overlay'}}
'PASS:38872/38873 goals/givers, all three39154 incoming alternatives, native catapult click/SmartAI, scoped Cullen migration repeat/custom preservation and Hotfix absence.'
