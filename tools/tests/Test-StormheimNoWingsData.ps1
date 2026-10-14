param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migration=Get-Content "$repo/sql/updates/world/2026_10_14_14_stormheim_no_wings.sql" -Raw
$sql=''
if(!$Live){
 $tables=@{creature_template='entry IN(91971,91975,91983,108403)';quest_objectives='QuestID IN(38318,38613,38410,38615)';quest_template_addon='ID IN(38318,38613,38410,38615)';quest_template='ID IN(38318,38613,38410,38615)';quest_start_scripts='id=38613 OR datalong=38613';npc_spellclick_spells='npc_entry IN(91975,91983,91979,92017)';smart_scripts='source_type=0 AND entryorguid IN(91975,91983,91481,92566)';spell_script_names='spell_id IN(182467,182468,182469,182478)'}
 $tables.conditions='(SourceTypeOrReferenceId=18 AND SourceGroup IN(91975,91983)) OR (SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=18)'
 foreach($table in $tables.Keys){$sql+="CREATE TEMPORARY TABLE saved_$table AS SELECT * FROM $table WHERE $($tables[$table]); CREATE TEMPORARY TABLE $table AS SELECT * FROM saved_$table;`n"}
 $sql+="UPDATE npc_spellclick_spells SET spell_id=121975 WHERE npc_entry IN(91975,91983); UPDATE quest_template_addon SET SpecialFlags=2 WHERE ID=38613; UPDATE quest_template SET StartScript=38613 WHERE ID=38613; DELETE FROM quest_start_scripts WHERE id=38613; INSERT INTO quest_start_scripts VALUES(38613,0,7,38613,0,0,0,0,0,0),(38613,0,7,38613,0,0,0,0,0,0);`n$migration`n$migration`n"
}
$sql+=@'
SELECT 'wall-objectives',COUNT(*) FROM quest_objectives WHERE Type=0 AND ObjectID=91484 AND Amount=1 AND StorageIndex=0 AND Flags=0 AND ((ID=277696 AND QuestID=38318) OR(ID=278221 AND QuestID=38613));
SELECT 'native-gun-click',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry IN(91975,91983) AND spell_id=182478 AND cast_flags=1 AND user_type=0 AND add_npc_flag=1;
SELECT 'no-click-credit',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid IN(91975,91983) AND event_type=73 AND action_type=33 AND action_param1=91484;
SELECT 'hook-script',COUNT(*) FROM creature_template WHERE entry=91971 AND AIName='' AND ScriptName='npc_stormheim_grapple_hook';
SELECT 'spell-handlers',COUNT(*) FROM spell_script_names WHERE (spell_id=182478 AND ScriptName='spell_stormheim_grapple_start') OR(spell_id=182467 AND ScriptName='spell_stormheim_grapple_summon') OR(spell_id=182468 AND ScriptName='aura_stormheim_grapple_hook') OR(spell_id=182469 AND ScriptName='spell_stormheim_grapple_jump');
SELECT 'normal-completion',COUNT(*) FROM quest_template_addon WHERE ID IN(38318,38613) AND (SpecialFlags&2)=0;
SELECT 'no-placeholder',COUNT(*) FROM quest_start_scripts WHERE id=38613 AND delay=0 AND command=7 AND datalong=38613;
SELECT 'empty-hook-detached',COUNT(*) FROM quest_template WHERE ID=38613 AND StartScript=0;
SELECT 'next-links',COUNT(*) FROM quest_template WHERE (ID=38318 AND RewardNextQuest=38410) OR(ID=38613 AND RewardNextQuest=38615);
SELECT 'point-flags-retained',COUNT(*) FROM creature_template WHERE entry IN(91975,91983) AND npcflag=16777216 AND unit_flags=768;
SELECT 'native-point-auras',COUNT(*) FROM creature_template_addon WHERE entry IN(91975,91983) AND auras='182476';
SELECT 'original-scout-actions',COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid IN(91481,92566) AND event_type=19 AND event_param1 IN(38318,38613) AND action_type=45 AND target_param2=108403;
SELECT 'next-prerequisites',COUNT(*) FROM quest_template_addon WHERE (ID=38410 AND PrevQuestID=38318) OR(ID=38615 AND PrevQuestID=38613);
SELECT 'alliance-recipient-phase',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=18 AND ConditionTypeOrReference=8 AND ConditionValue1=38312 AND NegativeCondition=0;
'@
$expected=@("wall-objectives`t2","native-gun-click`t2","no-click-credit`t0","hook-script`t1","spell-handlers`t4","normal-completion`t2","no-placeholder`t0","empty-hook-detached`t1","next-links`t2","point-flags-retained`t2","native-point-auras`t2","original-scout-actions`t2","next-prerequisites`t2","alliance-recipient-phase`t1")
if(!$Live){
 $sql+=@"
SELECT 'preserved-quests',COUNT(*) FROM quest_template a JOIN saved_quest_template b USING(ID) WHERE a.Flags<>b.Flags OR a.FlagsEx<>b.FlagsEx OR a.RewardNextQuest<>b.RewardNextQuest OR a.AllowableRaces<>b.AllowableRaces;
SELECT 'preserved-objectives',COUNT(*) FROM quest_objectives a JOIN saved_quest_objectives b USING(ID) WHERE a.Type<>b.Type OR a.ObjectID<>b.ObjectID OR a.Flags<>b.Flags;
SELECT 'other-points',COUNT(*) FROM npc_spellclick_spells a JOIN saved_npc_spellclick_spells b USING(npc_entry,spell_id) WHERE a.npc_entry IN(91979,92017) AND a.cast_flags=b.cast_flags;
UPDATE creature_template SET ScriptName='custom' WHERE entry=91971;
UPDATE quest_template_addon SET SpecialFlags=9 WHERE ID=38613;
INSERT INTO quest_start_scripts VALUES(38613,1,7,38613,0,0,0,0,0,0);
UPDATE quest_template SET StartScript=38613 WHERE ID=38613;
$migration
SELECT 'custom-hook',COUNT(*) FROM creature_template WHERE entry=91971 AND ScriptName='custom';
SELECT 'custom-flags',COUNT(*) FROM quest_template_addon WHERE ID=38613 AND SpecialFlags=9;
SELECT 'custom-start',COUNT(*) FROM quest_template WHERE ID=38613 AND StartScript=38613;
SELECT 'custom-action',COUNT(*) FROM quest_start_scripts WHERE id=38613 AND delay=1 AND command=7;
UPDATE quest_template_addon SET SpecialFlags=2 WHERE ID=38613;
$migration
SELECT 'custom-explore-flag',COUNT(*) FROM quest_template_addon WHERE ID=38613 AND SpecialFlags=2;
INSERT INTO npc_spellclick_spells VALUES(91975,121975,1,0,1);
$migration
SELECT 'existing-custom-click',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=91975;
DELETE FROM npc_spellclick_spells WHERE npc_entry=91975 AND spell_id=182478;
INSERT INTO conditions VALUES(18,91975,121975,0,0,2,0,138111,1,0,0,0,'','test custom click condition');
$migration
SELECT 'custom-conditioned-click',COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=91975 AND spell_id=121975;
"@
 $expected+=@("preserved-quests`t0","preserved-objectives`t0","other-points`t2","custom-hook`t1","custom-flags`t1","custom-start`t1","custom-action`t1","custom-explore-flag`t1","existing-custom-click`t2","custom-conditioned-click`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'No Wings SQL failed'}}finally{$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "No Wings SQL mismatch: $($actual -join '|')"}
'PASS: migration replay, native gun/hook bindings, exact mandatory objectives, no click/start credit and preserved other points, quest metadata, custom hooks/actions/flags.'
