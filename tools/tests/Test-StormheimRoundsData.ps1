param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_13_13_stormheim_rounds.sql" -Raw
$sql=''
if(!$Live){
 foreach($table in @('smart_scripts','creature_template','gameobject_template','spell_script_names','conditions')){
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 $sql+="UPDATE smart_scripts SET target_type=1 WHERE entryorguid=96686 AND source_type=9 AND id=4;`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'guides',COUNT(*) FROM creature_template WHERE entry IN(96644,96686) AND ScriptName='npc_stormheim_introduction' AND AIName='SmartAI';
SELECT 'finals',COUNT(*) FROM creature_template WHERE entry IN(96663,96689) AND ScriptName='npc_stormheim_rounds_final' AND AIName='SmartAI' AND npcflag IN(1,2);
SELECT 'catapult',COUNT(*) FROM gameobject_template WHERE entry=247491 AND type=10 AND ScriptName='go_stormheim_inspect_catapult';
SELECT 'bindings',COUNT(*) FROM spell_script_names WHERE (spell_id IN(69333,69334) AND ScriptName='spell_stormheim_rounds_start') OR (spell_id IN(220525,220291) AND ScriptName='spell_stormheim_rounds_salute') OR (spell_id=220521 AND ScriptName='spell_stormheim_inspect_gun');
SELECT 'self-bar',COUNT(*) FROM smart_scripts WHERE entryorguid=96686 AND source_type=9 AND id=4 AND action_type=85 AND action_param1=220293 AND target_type=23;
SELECT 'scoped-summons',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=5 AND SourceId=0 AND ElseGroup=0 AND ((SourceEntry=96644 AND ((ConditionTypeOrReference=22 AND ConditionTarget=0 AND ConditionValue1=0) OR (ConditionTypeOrReference=9 AND ConditionTarget=1 AND ConditionValue1=38206))) OR (SourceEntry=96686 AND ((ConditionTypeOrReference=22 AND ConditionTarget=0 AND ConditionValue1=1) OR(ConditionTypeOrReference=9 AND ConditionTarget=1 AND ConditionValue1=39698))));
SELECT 'objective-count',COUNT(*) FROM quest_objectives WHERE QuestID IN(38206,39698);
SELECT 'native-objectives',COUNT(*) FROM quest_objectives WHERE Amount=1 AND Flags2=0 AND ((ID=279839 AND QuestID=38206 AND Type=0 AND StorageIndex=0 AND ObjectID=96662 AND Flags=0) OR(ID=279840 AND QuestID=38206 AND Type=0 AND StorageIndex=1 AND ObjectID=96661 AND Flags=2) OR(ID=279841 AND QuestID=38206 AND Type=0 AND StorageIndex=2 AND ObjectID=96663 AND Flags=2) OR(ID=279849 AND QuestID=39698 AND Type=2 AND StorageIndex=0 AND ObjectID=247491 AND Flags=0) OR(ID=279850 AND QuestID=39698 AND Type=0 AND StorageIndex=1 AND ObjectID=96688 AND Flags=2) OR(ID=279851 AND QuestID=39698 AND Type=0 AND StorageIndex=2 AND ObjectID=96689 AND Flags=2));
SELECT 'next',COUNT(*) FROM quest_template_addon WHERE (ID=38206 AND PrevQuestID=38035 AND NextQuestID=39800) OR(ID=39698 AND PrevQuestID=38307) OR(ID=39800 AND PrevQuestID=38206) OR(ID=39801 AND PrevQuestID=39698);
SELECT 'gun-click',COUNT(*) FROM npc_spellclick_spells s JOIN creature_template t ON t.entry=s.npc_entry WHERE s.npc_entry=110898 AND s.spell_id=220521 AND s.cast_flags=1 AND (t.npcflag & 16777216)<>0;
SELECT 'gun-hit-credit',COUNT(*) FROM smart_scripts WHERE entryorguid=110898 AND source_type=0 AND event_type=8 AND event_param1=220521 AND action_type=33 AND action_param1=96662 AND target_type=7;
SELECT 'gun-ai',COUNT(*) FROM creature_template WHERE entry=110898 AND AIName='' AND ScriptName='';
SELECT 'inspection-spawns',COUNT(*) FROM gameobject WHERE guid=127743 AND id=247491 AND map=1 AND PhaseId='5968';
SELECT 'gun-spawn',COUNT(*) FROM creature WHERE guid=146728268 AND id=110898 AND map=0 AND PhaseId='6649';
SELECT 'paths',COUNT(*),COUNT(DISTINCT entry) FROM waypoints WHERE entry IN(966440,966441,966442,966860,966861,966862);
SELECT 'contiguous-paths',COUNT(*) FROM (SELECT entry FROM waypoints WHERE entry IN(966440,966441,966442,966860,966861,966862) GROUP BY entry HAVING MIN(pointid)=1 AND MAX(pointid)=COUNT(*)) paths;
SELECT 'native-starts',COUNT(*) FROM event_scripts WHERE (id=22506 AND command=10 AND datalong=96644 AND datalong2=300000) OR (id=22507 AND command=10 AND datalong=96686 AND datalong2=300000);
SELECT 'ranger-formation',COUNT(*) FROM event_scripts WHERE (id=53130 AND command=10 AND datalong=92670 AND datalong2=10000) OR(id=53167 AND command=10 AND datalong=91950 AND datalong2=10000);
"@
$expected=@("guides`t2","finals`t2","catapult`t1","bindings`t5","self-bar`t1","scoped-summons`t4","objective-count`t6","native-objectives`t6","next`t4","gun-click`t1","gun-hit-credit`t0","gun-ai`t1","inspection-spawns`t1","gun-spawn`t1","paths`t59`t6","contiguous-paths`t6","native-starts`t2","ranger-formation`t16")
if(!$Live){
 $sql+=@"
SELECT 'other-actors',COUNT(*) FROM creature_template a JOIN saved_creature_template b USING(entry) WHERE a.entry NOT IN(96686,96663,96689) AND a.ScriptName<>b.ScriptName;
SELECT 'other-objects',COUNT(*) FROM gameobject_template a JOIN saved_gameobject_template b USING(entry) WHERE a.entry<>247491 AND a.ScriptName<>b.ScriptName;
SELECT 'other-smart',COUNT(*) FROM smart_scripts a JOIN saved_smart_scripts b USING(entryorguid,source_type,id,link) WHERE NOT(a.entryorguid=96686 AND a.source_type=9 AND a.id=4) AND a.target_type<>b.target_type;
UPDATE creature_template SET ScriptName='custom' WHERE entry=96686;
UPDATE gameobject_template SET ScriptName='custom' WHERE entry=247491;
"@
 $sql+="$migration`nSELECT 'custom-npc',COUNT(*) FROM creature_template WHERE entry=96686 AND ScriptName='custom'; SELECT 'custom-go',COUNT(*) FROM gameobject_template WHERE entry=247491 AND ScriptName='custom';"
 $expected+=@("other-actors`t0","other-objects`t0","other-smart`t0","custom-npc`t1","custom-go`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names
 if($LASTEXITCODE){throw 'Rounds SQL test failed'}
} finally {$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Rounds SQL mismatch: $($actual -join '|')"}
'PASS: both native rounds chains, six objectives/routes, gun cast/hit and GO click, player-targeted bar, next quests, spawn/flag data and isolated/idempotent bindings.'
