param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_14_01_stormheim_followup.sql" -Raw
$sql=''
if(!$Live){
 foreach($table in @('creature_template','gameobject_template','quest_objectives','spell_script_names','conditions','creature')){
  $filter=if($table -eq 'creature'){' WHERE id=90948 OR guid IN(340416,341176,340422,341180,290451)'}else{''}
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table$filter; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 # MySQL cannot self-join one temporary table. Redirect only the unchanged
 # original-spawn alias to its identical read-only snapshot for this test.
 $temporaryMigration=$migration.Replace('JOIN creature original','JOIN saved_creature original')
 $sql+="CREATE TEMPORARY TABLE duplicate_seed LIKE creature; INSERT INTO duplicate_seed SELECT * FROM saved_creature WHERE guid=340416; UPDATE duplicate_seed SET guid=341176; INSERT INTO creature SELECT * FROM duplicate_seed ON DUPLICATE KEY UPDATE guid=VALUES(guid);`n"
 $sql+="UPDATE quest_objectives SET ObjectID=35841 WHERE ID=277453; UPDATE creature_template SET ScriptName='npc_amberbeam_stalker' WHERE entry=90903;`n$temporaryMigration`n$temporaryMigration`n"
}
$sql+=@"
SELECT 'native-charge',COUNT(*) FROM quest_objectives WHERE (ID=277453 AND QuestID=38058 AND Type=0 AND StorageIndex=0 AND ObjectID=90866 AND Amount=1 AND Flags=0) OR(ID=277457 AND QuestID=38058 AND Type=0 AND StorageIndex=1 AND ObjectID=90903 AND Amount=100 AND Flags=12);
SELECT 'native-clues',COUNT(*) FROM quest_objectives WHERE QuestID=38360 AND Amount=1 AND ((ID=277870 AND ObjectID=241031 AND Type=2 AND Flags=0) OR(ID=277871 AND ObjectID=91904 AND Type=0 AND Flags=2) OR(ID=277872 AND ObjectID=241032 AND Type=2 AND Flags=2));
SELECT 'native-rescue',COUNT(*) FROM quest_objectives WHERE ID=277455 AND QuestID=38059 AND Type=0 AND ObjectID=90948 AND Amount=6 AND Flags=0;
SELECT 'actor-scripts',COUNT(*) FROM creature_template WHERE entry IN(90903,90948,91904) AND ScriptName='npc_stormheim_followup_actor';
SELECT 'clue-scripts',COUNT(*) FROM gameobject_template WHERE entry IN(241031,241032) AND type=10 AND Data0=1690 AND Data1=38360 AND Data10=0 AND ScriptName='go_stormheim_followup_clue';
SELECT 'spell-scripts',COUNT(*) FROM spell_script_names WHERE (spell_id IN(180737,182313) AND ScriptName='spell_stormheim_followup_click') OR(spell_id IN(180677,183386,183454) AND ScriptName='spell_stormheim_followup_lightning');
SELECT 'native-clicks',COUNT(*) FROM npc_spellclick_spells WHERE cast_flags=1 AND user_type=0 AND add_npc_flag=1 AND ((npc_entry=90948 AND spell_id=180737) OR(npc_entry=91904 AND spell_id=182313));
SELECT 'click-conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup IN(90948,91904) AND Comment LIKE 'Stormheim followup - %';
SELECT 'active-rescue-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=90948 AND SourceEntry=180737 AND ConditionTypeOrReference=9 AND ConditionValue1=38059 AND NegativeCondition=0;
SELECT 'captain-first-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=91904 AND SourceEntry=182313 AND ConditionTypeOrReference=41 AND ConditionValue1=38360 AND ConditionValue2=241031 AND ConditionValue3=1 AND NegativeCondition=0;
SELECT 'captain-repeat-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=91904 AND SourceEntry=182313 AND ConditionTypeOrReference=41 AND ConditionValue1=38360 AND ConditionValue2=91904 AND ConditionValue3=1 AND NegativeCondition=1;
SELECT 'native-pose',COUNT(*) FROM creature_template WHERE entry=90948 AND SpellStateVisualID=45944 AND SpellStateAnimKitID=6050;
SELECT 'native-scaling',COUNT(*) FROM creature_template_scaling WHERE Entry IN(90903,90948,91904) AND LevelScalingMin=98 AND LevelScalingMax=110;
SELECT 'original-captain',COUNT(*) FROM creature WHERE guid=340416 AND id=91904 AND map=1220;
SELECT 'duplicate-captain',COUNT(*) FROM creature WHERE guid=341176 AND id=91904;
SELECT 'rescue-spawns',COUNT(*) FROM creature WHERE id=90948 AND map=1220 AND phaseMask=1;
SELECT 'clue-spawns',COUNT(*) FROM gameobject WHERE id IN(241031,241032) AND map=1220 AND phaseMask=1;
SELECT 'prerequisites',COUNT(*) FROM quest_template_addon WHERE (ID=38058 AND PrevQuestID=38558) OR(ID=38059 AND PrevQuestID=38057) OR(ID=38360 AND PrevQuestID=38332);
SELECT 'incoming-battery',(SELECT COUNT(*) FROM quest_template_addon WHERE ID=38053 AND NextQuestID=38058);
SELECT 'native-ui-completion',COUNT(*) FROM quest_template WHERE ID IN(38058,38059) AND (Flags & 65536)<>0;
SELECT 'horde-ender',COUNT(*) FROM creature_questender WHERE id=91158 AND quest=38360;
SELECT 'next-links',COUNT(*) FROM quest_template_addon WHERE (ID IN(38058,38059) AND NextQuestID=38060) OR(ID=38360 AND NextQuestID=38362);
SELECT 'original-nathanos',COUNT(*) FROM creature WHERE guid=340422 AND id=91158;
SELECT 'duplicate-nathanos',COUNT(*) FROM creature WHERE guid=341180;
"@
$expected=@("native-charge`t2","native-clues`t3","native-rescue`t1","actor-scripts`t3","clue-scripts`t2","spell-scripts`t5","native-clicks`t2","click-conditions`t8","active-rescue-condition`t1","captain-first-condition`t1","captain-repeat-condition`t1","native-pose`t1","native-scaling`t3","original-captain`t1","duplicate-captain`t0","rescue-spawns`t6","clue-spawns`t2","prerequisites`t3","incoming-battery`t1","native-ui-completion`t2","horde-ender`t1","next-links`t3","original-nathanos`t1","duplicate-nathanos`t0")
if(!$Live){
 $sql+=@"
SELECT 'other-actors',COUNT(*) FROM creature_template a JOIN saved_creature_template b USING(entry) WHERE a.entry NOT IN(90903,90948,91904) AND a.ScriptName<>b.ScriptName;
SELECT 'other-objectives',COUNT(*) FROM quest_objectives a JOIN saved_quest_objectives b USING(ID) WHERE a.ID<>277453 AND a.ObjectID<>b.ObjectID;
SELECT 'other-spawns',COUNT(*) FROM saved_creature a LEFT JOIN creature b USING(guid) WHERE a.guid NOT IN(341176,341180) AND b.guid IS NULL;
UPDATE creature_template SET ScriptName='custom' WHERE entry=90903;
UPDATE gameobject_template SET ScriptName='custom' WHERE entry=241031;
INSERT INTO creature SELECT * FROM duplicate_seed;
UPDATE creature SET PhaseId='999' WHERE guid=341176;
"@
 $sql+="$temporaryMigration`nSELECT 'custom-actor',COUNT(*) FROM creature_template WHERE entry=90903 AND ScriptName='custom'; SELECT 'custom-object',COUNT(*) FROM gameobject_template WHERE entry=241031 AND ScriptName='custom'; SELECT 'custom-spawn',COUNT(*) FROM creature WHERE guid=341176 AND PhaseId='999';"
 $expected+=@("other-actors`t0","other-objectives`t0","other-spawns`t0","custom-actor`t1","custom-object`t1","custom-spawn`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'Followup SQL failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Followup SQL mismatch: $($actual -join '|')"}
'PASS: native charge/rescue/sequenced clues, executable bindings/conditions, native pose/scaling, six rescues/two clues, guarded duplicate removal, UI/hand-in/next graph and custom/unrelated preservation.'
