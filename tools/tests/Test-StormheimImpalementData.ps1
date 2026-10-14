param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_14_15_stormheim_impalement.sql" -Raw
$sql=''
if(!$Live){
 $tables=@{gameobject_template='entry=251257';quest_objectives='QuestID IN(38410,38615)';quest_template='ID IN(38410,38615,38342,38617)';quest_template_addon='ID IN(38410,38615,38342,38617)';quest_start_scripts='id=38615 OR datalong=38615';phase_definitions='zoneId=7541 AND entry=127';conditions='SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=127'}
 foreach($table in $tables.Keys){$sql+="CREATE TEMPORARY TABLE saved_$table AS SELECT * FROM $table WHERE $($tables[$table]); CREATE TEMPORARY TABLE $table AS SELECT * FROM saved_$table;`n"}
 $sql+="DELETE FROM conditions WHERE ElseGroup=1; UPDATE quest_template SET StartScript=38615 WHERE ID=38615; UPDATE quest_template_addon SET SpecialFlags=2 WHERE ID=38615; DELETE FROM quest_start_scripts WHERE id=38615; INSERT INTO quest_start_scripts VALUES(38615,0,7,38615,0,0,0,0,0,0),(38615,0,7,38615,0,0,0,0,0,0);`n$migration`n$migration`n"
}
$sql+=@'
SELECT 'objectives',COUNT(*) FROM quest_objectives WHERE QuestID IN(38410,38615) AND Type=2 AND ObjectID=251257 AND Amount=7 AND Flags=0 AND StorageIndex=0;
SELECT 'object-binding',COUNT(*) FROM gameobject_template WHERE entry=251257 AND type=10 AND Data0=43 AND Data10=0 AND Data14=80413 AND ScriptName='go_stormheim_impalement';
SELECT 'phase-branches',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=127 AND SourceId=0 AND ConditionTarget=0 AND ConditionTypeOrReference=9 AND ((ElseGroup=0 AND ConditionValue1=38410 AND NegativeCondition=0) OR(ElseGroup=1 AND ConditionValue1=38615 AND NegativeCondition=0) OR(ElseGroup IN(0,1) AND ConditionValue1=41451 AND NegativeCondition=1));
SELECT 'native-phase',COUNT(*) FROM phase_definitions WHERE zoneId=7541 AND entry=127 AND phaseId='6779' AND flags=16;
SELECT 'normal-quests',COUNT(*) FROM quest_template_addon WHERE ID IN(38410,38615) AND (SpecialFlags&2)=0;
SELECT 'no-start-placeholder',COUNT(*) FROM quest_start_scripts WHERE id=38615 AND delay=0 AND command=7 AND datalong=38615;
SELECT 'empty-start-detached',COUNT(*) FROM quest_template WHERE ID=38615 AND StartScript=0;
SELECT 'next-links',COUNT(*) FROM quest_template WHERE (ID=38410 AND RewardNextQuest=38342) OR(ID=38615 AND RewardNextQuest=38617);
SELECT 'next-prerequisites',COUNT(*) FROM quest_template_addon WHERE (ID=38342 AND PrevQuestID=38410) OR(ID=38617 AND PrevQuestID=38615);
SELECT 'launcher-spawns',COUNT(*) FROM gameobject WHERE id=251257 AND map=1220 AND zoneId=7541 AND FIND_IN_SET('6779',REPLACE(PhaseId,' ',','));
SELECT 'live-horde-recipient',COUNT(*) FROM creature WHERE id=92566 AND map=1220 AND zoneId=7541;
SELECT 'live-horde-next',COUNT(*) FROM creature_queststarter WHERE id=92566 AND quest=38617;
SELECT 'live-horde-hand-in',COUNT(*) FROM creature_questender WHERE id=92566 AND quest=38615;
'@
$expected=@("objectives`t2","object-binding`t1","phase-branches`t4","native-phase`t1","normal-quests`t2","no-start-placeholder`t0","empty-start-detached`t1","next-links`t2","next-prerequisites`t2","launcher-spawns`t17","live-horde-recipient`t2","live-horde-next`t1","live-horde-hand-in`t1")
if(!$Live){
 $sql+=@"
SELECT 'preserved-fields',COUNT(*) FROM gameobject_template a JOIN saved_gameobject_template b USING(entry) WHERE a.Data3<>b.Data3 OR a.Data5<>b.Data5 OR a.Data10<>b.Data10 OR a.Data14<>b.Data14 OR a.Data20<>b.Data20 OR a.displayId<>b.displayId OR a.flags<>b.flags;
SELECT 'preserved-quest-fields',COUNT(*) FROM quest_template a JOIN saved_quest_template b USING(ID) WHERE a.Flags<>b.Flags OR a.FlagsEx<>b.FlagsEx OR a.RewardNextQuest<>b.RewardNextQuest OR a.AllowableRaces<>b.AllowableRaces;
UPDATE gameobject_template SET ScriptName='custom' WHERE entry=251257;
UPDATE quest_template_addon SET SpecialFlags=9 WHERE ID=38615;
INSERT INTO quest_start_scripts VALUES(38615,1,7,38615,0,0,0,0,0,0);
UPDATE quest_template SET StartScript=38615 WHERE ID=38615;
DELETE FROM conditions WHERE ElseGroup=1;
INSERT INTO conditions VALUES(23,7541,127,0,1,9,0,999,0,0,0,0,'','custom branch');
$migration
SELECT 'custom-binding',COUNT(*) FROM gameobject_template WHERE entry=251257 AND ScriptName='custom';
SELECT 'custom-flags',COUNT(*) FROM quest_template_addon WHERE ID=38615 AND SpecialFlags=9;
SELECT 'custom-start',COUNT(*) FROM quest_template WHERE ID=38615 AND StartScript=38615;
SELECT 'custom-action',COUNT(*) FROM quest_start_scripts WHERE id=38615 AND delay=1 AND command=7;
SELECT 'custom-phase-branch',COUNT(*) FROM conditions WHERE ElseGroup=1;
"@
 $expected+=@("preserved-fields`t0","preserved-quest-fields`t0","custom-binding`t1","custom-flags`t1","custom-start`t1","custom-action`t1","custom-phase-branch`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'Impalement SQL failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Impalement SQL mismatch: $($actual -join '|')"}
'PASS: migration replay, two mandatory seven-launcher quests, protected object metadata, both phase branches/exclusion,17 spawns, no Horde start completion, next prerequisites and custom preservation.'
