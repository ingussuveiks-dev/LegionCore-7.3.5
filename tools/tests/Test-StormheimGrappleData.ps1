param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_14_13_stormheim_grapple_pickup.sql" -Raw
$sql=''
if(!$Live){
 $tables=@{creature_template='entry IN(91222,91481,92561,92566,107840)';quest_objectives='QuestID IN(38312,38612,38318,38613)';quest_template_addon='ID IN(38312,38612,38318,38613,39775)';quest_template='ID IN(38312,38612,38318,38613,39775)';quest_start_scripts='id=38612 OR datalong=38612'}
 foreach($table in $tables.Keys){$sql+="CREATE TEMPORARY TABLE saved_$table AS SELECT * FROM $table WHERE $($tables[$table]); CREATE TEMPORARY TABLE $table AS SELECT * FROM saved_$table;`n"}
 $sql+="UPDATE quest_objectives SET Type=1,ObjectID=138111,Flags=4 WHERE ID IN(277684,278219); UPDATE quest_template_addon SET SpecialFlags=2 WHERE ID=38612; UPDATE quest_template SET StartScript=38612 WHERE ID=38612; DELETE FROM quest_start_scripts WHERE id=38612; INSERT INTO quest_start_scripts VALUES(38612,0,7,38612,0,0,0,0,0,0),(38612,0,7,38612,0,0,0,0,0,0),(38612,0,7,38612,0,0,0,0,0,0),(38612,0,7,38612,0,0,0,0,0,0);`n$migration`n$migration`n"
}
$sql+=@'
SELECT 'native-pickup-objectives',COUNT(*) FROM quest_objectives WHERE Type=14 AND Amount=1 AND StorageIndex=0 AND Flags=0 AND ((ID=277684 AND QuestID=38312 AND ObjectID=55241) OR(ID=278219 AND QuestID=38612 AND ObjectID=55243));
SELECT 'supply-actors',COUNT(*) FROM creature_template WHERE entry IN(91222,91481,92561,92566) AND AIName='' AND ScriptName='npc_stormheim_grapple_supply';
SELECT 'normal-completion',COUNT(*) FROM quest_template_addon WHERE ID IN(38312,38612) AND (SpecialFlags&2)=0;
SELECT 'no-placeholder-completion',COUNT(*) FROM quest_start_scripts WHERE id=38612 AND command=7 AND datalong=38612;
SELECT 'detached-empty-start',COUNT(*) FROM quest_template WHERE ID=38612 AND StartScript=0 AND RewardSpell=191831;
SELECT 'pickup-native',COUNT(*) FROM quest_template WHERE ID=39775 AND QuestType=0 AND Flags=37748744 AND RewardItem1=138111 AND RewardAmount1=1;
SELECT 'physical-pickup-relations',COUNT(*) FROM creature_queststarter WHERE id=107840 AND quest=39775;
SELECT 'physical-pickup-visual',COUNT(*) FROM creature_template_addon WHERE entry=107840 AND auras='214101';
SELECT 'retained-smart-grants',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid IN(91222,92561) AND action_type=56 AND action_param1=138111 AND action_param2=1;
SELECT 'next-source-reuse',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid IN(91481,92566) AND event_type=19 AND event_param1 IN(38318,38613) AND action_type=45 AND target_type=10 AND target_param2=108403;
SELECT 'next-scope-not-repaired',COUNT(*) FROM quest_template WHERE ID=38613 AND StartScript=38613;
SELECT 'preserved-races',COUNT(*) FROM quest_template WHERE (ID=38312 AND AllowableRaces=824181837) OR(ID=38612 AND AllowableRaces=234881970);
'@
$expected=@("native-pickup-objectives`t2","supply-actors`t4","normal-completion`t2","no-placeholder-completion`t0","detached-empty-start`t1","pickup-native`t1","physical-pickup-relations`t1","physical-pickup-visual`t1","retained-smart-grants`t2","next-source-reuse`t2","next-scope-not-repaired`t1","preserved-races`t2")
if(!$Live){
 $sql+=@"
SELECT 'preserved-actors',COUNT(*) FROM creature_template a JOIN saved_creature_template b USING(entry) WHERE a.npcflag<>b.npcflag OR a.unit_flags<>b.unit_flags OR a.faction<>b.faction OR (a.entry=107840 AND (a.AIName<>b.AIName OR a.ScriptName<>b.ScriptName));
SELECT 'preserved-next-objectives',COUNT(*) FROM quest_objectives a JOIN saved_quest_objectives b USING(ID) WHERE a.QuestID IN(38318,38613) AND (a.Type<>b.Type OR a.ObjectID<>b.ObjectID OR a.Flags<>b.Flags);
SELECT 'preserved-quest-flags',COUNT(*) FROM quest_template a JOIN saved_quest_template b USING(ID) WHERE a.Flags<>b.Flags OR a.FlagsEx<>b.FlagsEx OR a.RewardNextQuest<>b.RewardNextQuest;
UPDATE quest_objectives SET Type=1,ObjectID=999,Flags=4 WHERE ID=277684;
UPDATE creature_template SET ScriptName='custom' WHERE entry=91222;
UPDATE quest_template_addon SET SpecialFlags=9 WHERE ID=38612;
$migration
SELECT 'custom-objective',COUNT(*) FROM quest_objectives WHERE ID=277684 AND ObjectID=999 AND Type=1;
SELECT 'custom-actor',COUNT(*) FROM creature_template WHERE entry=91222 AND ScriptName='custom';
SELECT 'custom-flags',COUNT(*) FROM quest_template_addon WHERE ID=38612 AND SpecialFlags=9;
INSERT INTO quest_start_scripts VALUES(38612,1,7,38612,0,0,0,0,0,0);
UPDATE quest_template SET StartScript=38612 WHERE ID=38612;
UPDATE quest_template_addon SET SpecialFlags=2 WHERE ID=38612;
$migration
SELECT 'custom-start-action',COUNT(*) FROM quest_start_scripts WHERE id=38612 AND delay=1 AND command=7;
SELECT 'custom-start-hook',COUNT(*) FROM quest_template WHERE ID=38612 AND StartScript=38612;
SELECT 'custom-explore-flag',COUNT(*) FROM quest_template_addon WHERE ID=38612 AND SpecialFlags=2;
"@
 $expected+=@("preserved-actors`t0","preserved-next-objectives`t0","preserved-quest-flags`t0","custom-objective`t1","custom-actor`t1","custom-flags`t1","custom-start-action`t1","custom-start-hook`t1","custom-explore-flag`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'Grapple SQL failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Grapple SQL mismatch: $($actual -join '|')"}
'PASS: migration twice, mandatory native pickup criteria, actual source/visual/grant/next data, no Horde placeholder and custom/unrelated preservation.'
