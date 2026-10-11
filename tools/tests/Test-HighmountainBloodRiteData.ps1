param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_11_06_highmountain_blood_rite.sql" -Raw
$native=Get-Content "$repo/docs/audits/highmountain-blood-rite-native-2026-10-11.json" -Raw | ConvertFrom-Json
$rows=($native.objectives | ForEach-Object {"SELECT $($_.ID) ID,$($_.QuestID) QuestID,$($_.Type) Type,$($_.StorageIndex) StorageIndex,$($_.ObjectID) ObjectID,$($_.Amount) Amount,$($_.Flags) Flags,$($_.Flags2) Flags2"}) -join ' UNION ALL '
$sql=''
if(!$Live){
 $sql=(@('creature_template','smart_scripts','conditions','npc_spellclick_spells','creature_text') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$migration`n$migration`n"
}
$sql+=@"
SELECT 'native-objectives',COUNT(*) FROM ($rows) a LEFT JOIN quest_objectives b USING(ID) WHERE b.ID IS NULL OR a.QuestID<>b.QuestID OR a.Type<>b.Type OR a.StorageIndex<>b.StorageIndex OR a.ObjectID<>b.ObjectID OR a.Amount<>b.Amount OR a.Flags<>b.Flags OR a.Flags2<>b.Flags2;
SELECT 'approach-binding',COUNT(*) FROM creature_template WHERE entry=95259 AND AIName='' AND ScriptName='npc_blood_rite_navarrogg' AND (npcflag & 16777216)=0;
SELECT 'boss-binding',COUNT(*) FROM creature_template WHERE entry=97418 AND AIName='' AND ScriptName='npc_blood_rite_marakhan' AND HealthScalingExpansion=6;
SELECT 'portal-binding',COUNT(*) FROM creature_template WHERE entry=97675 AND AIName='NullCreatureAI' AND ScriptName='';
SELECT 'shared-scene',COUNT(*) FROM smart_scripts WHERE (source_type=0 AND entryorguid IN(95259,97418,97675)) OR (source_type=9 AND entryorguid IN(9741800,9767500));
SELECT 'shared-blocker',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=95259 AND SourceId=0;
SELECT 'old-click',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=95259 AND spell_id=192482;
SELECT 'combat-spells',COUNT(*) FROM creature_template_spell WHERE entry=97418 AND spell IN(197670,197681);
SELECT 'boss-scaling',COUNT(*) FROM creature_template_scaling WHERE Entry=97418 AND LevelScalingMin=98 AND LevelScalingMax=110;
SELECT 'sacrifice',COUNT(*) FROM smart_scripts WHERE entryorguid=97440 AND source_type=0 AND event_type=8 AND event_param1=212758 AND action_type=37 AND target_type=1;
SELECT 'acceptance',COUNT(*) FROM smart_scripts WHERE entryorguid=93836 AND source_type=0 AND event_type=19 AND event_param1=39860 AND action_type=85 AND action_param1=190902;
SELECT 'guide-area',COUNT(*) FROM spell_area WHERE spell=192536 AND area=7807 AND quest_start=39860 AND quest_end=39381 AND quest_start_status=66 AND quest_end_status=66 AND autocast=1;
SELECT 'guide-ender',COUNT(*) FROM creature_questender WHERE id=96286 AND quest=39860;
SELECT 'guide-next',COUNT(*) FROM creature_queststarter WHERE id=96286 AND quest=39381;
SELECT 'next-gate',COUNT(*) FROM quest_template_addon WHERE ID=39381 AND PrevQuestID=39860;
SELECT 'guide-ai',COUNT(*) FROM creature_template WHERE entry=96286 AND AIName='' AND ScriptName='npc_rock_escape_navarrogg' AND (npcflag & 2)=2;
SELECT 'escape',COUNT(*) FROM smart_scripts WHERE entryorguid=9628600 AND source_type=9;
SELECT 'actors-position',COUNT(*) FROM creature WHERE (id=95259 AND guid=283395 AND map=1220 AND ABS(position_x-5110.94)<0.02 AND ABS(position_y-5071.74)<0.02) OR (id=97675 AND guid=283448 AND map=1220 AND ABS(position_x-5103.12)<0.02 AND ABS(position_y-5073.15)<0.02);
SELECT 'dialogue',COUNT(*) FROM creature_text WHERE CreatureID=97418 AND ((GroupID=0 AND Text='More souls! Come and accept your fate!' AND BroadcastTextID=99494) OR (GroupID=1 AND Text='Me rip, you die!' AND BroadcastTextID=82906));
SELECT 'no-disable',COUNT(*) FROM disables WHERE sourceType=1 AND entry IN(39860,39381);
"@
$expected=@("native-objectives`t0","approach-binding`t1","boss-binding`t1","portal-binding`t1","shared-scene`t0","shared-blocker`t0","old-click`t0","combat-spells`t2","boss-scaling`t1","sacrifice`t1","acceptance`t1","guide-area`t1","guide-ender`t1","guide-next`t1","next-gate`t1","guide-ai`t1","escape`t0","actors-position`t2","dialogue`t2","no-disable`t0")
if(!$Live){
 $sql+=@"
SELECT 'unrelated-bindings',COUNT(*) FROM saved_creature_template a JOIN creature_template b USING(entry) WHERE (a.entry NOT IN(95259,97418,97675) AND (a.AIName<>b.AIName OR a.ScriptName<>b.ScriptName)) OR (a.entry<>95259 AND a.npcflag<>b.npcflag) OR a.unit_flags<>b.unit_flags OR a.faction<>b.faction OR a.HealthScalingExpansion<>b.HealthScalingExpansion;
SELECT 'unrelated-smart', (SELECT COUNT(*) FROM saved_smart_scripts WHERE NOT ((source_type=0 AND entryorguid IN(95259,97418,97675)) OR (source_type=9 AND entryorguid IN(9741800,9767500))))-(SELECT COUNT(*) FROM smart_scripts);
SELECT 'unrelated-click', (SELECT COUNT(*) FROM saved_npc_spellclick_spells WHERE NOT(npc_entry=95259 AND spell_id=192482))-(SELECT COUNT(*) FROM npc_spellclick_spells);
SELECT 'unrelated-conditions',(SELECT COUNT(*) FROM saved_conditions WHERE NOT(SourceTypeOrReferenceId=22 AND SourceEntry=95259 AND SourceId=0))-(SELECT COUNT(*) FROM conditions);
"@
 $expected+=@("unrelated-bindings`t0","unrelated-smart`t0","unrelated-click`t0","unrelated-conditions`t0")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=& 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names --execute=$sql;if($LASTEXITCODE){throw 'Blood Rite SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Blood Rite SQL mismatch: $($actual -join '|')"}
'PASS: twice-applied migration, native ordered objectives, personal scene bindings, preserved combat/scaling/companion/guide/escape/next gate and unrelated data.'
