param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$migration=Get-Content "$repo/sql/updates/world/2026_10_12_20_highmountain_evacuation.sql" -Raw
$hotfix=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='}).Split('"')[1].Split(';')[4]
$sql=''
if(!$Live){
 foreach($table in @('creature_template','creature_text','gossip_menu_option','smart_scripts','conditions')){
  $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
 }
 $sql+="$migration`n$migration`n"
}
$sql+=@"
SELECT 'ai',COUNT(*) FROM creature_template WHERE entry IN(106115,106153,106191) AND AIName='' AND ScriptName='npc_highmountain_evacuation';
SELECT 'obsolete-actions',COUNT(*) FROM smart_scripts WHERE (source_type=0 AND entryorguid IN(106115,106153,106191)) OR (source_type=9 AND entryorguid IN(10611500,10615300,10619100));
SELECT 'texts',COUNT(*) FROM creature_text WHERE GroupID=0 AND ID=0 AND ((CreatureID=106115 AND BroadcastTextID=109069) OR (CreatureID=106153 AND BroadcastTextID=109086) OR (CreatureID=106191 AND BroadcastTextID=109107)) AND Text NOT LIKE '%?%';
SELECT 'options',COUNT(*) FROM gossip_menu_option WHERE MenuID IN(19560,19561,19565) AND OptionID=0 AND OptionBroadcastTextID=109076 AND OptionText='There is no time left. Run!';
SELECT 'active',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup IN(19560,19561,19565) AND SourceEntry=0 AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=9 AND ConditionValue1=42088 AND NegativeCondition=0;
SELECT 'uncredited',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceEntry=0 AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=41 AND ConditionValue1=42088 AND ConditionValue3=1 AND NegativeCondition=1 AND ((SourceGroup=19560 AND ConditionValue2=106115) OR (SourceGroup=19561 AND ConditionValue2=106153) OR (SourceGroup=19565 AND ConditionValue2=106191));
SELECT 'objective-shape',COUNT(*) FROM quest_objectives WHERE QuestID=42088 AND Type=0 AND Amount=1 AND Flags=0 AND ((ID=283606 AND ObjectID=106115 AND StorageIndex=0) OR (ID=283617 AND ObjectID=106153 AND StorageIndex=1) OR (ID=283622 AND ObjectID=106191 AND StorageIndex=2));
SELECT 'spawns',COUNT(*) FROM creature WHERE (guid=11353707 AND id=106115 AND ABS(position_x-3820.42)<0.01 AND ABS(position_y-4011.6)<0.01 AND ABS(position_z-898.375)<0.01) OR (guid=11353709 AND id=106153 AND ABS(position_x-3799.07)<0.01 AND ABS(position_y-3919.26)<0.01 AND ABS(position_z-915.923)<0.01) OR (guid=284846 AND id=106191);
SELECT 'spawn-phase',COUNT(*) FROM creature WHERE id IN(106115,106153,106191) AND map=1220 AND phaseId='9180';
SELECT 'mayla-start',COUNT(*) FROM creature_queststarter WHERE id=93846 AND quest=42088;
SELECT 'mayla-end',COUNT(*) FROM creature_questender WHERE id=93846 AND quest=42088;
SELECT 'nefu-aura',COUNT(*) FROM creature_template_addon WHERE entry=106191 AND auras='130966';
SELECT 'carry-spells',COUNT(*) FROM creature_template_spell WHERE (entry=106115 AND spell=210246) OR (entry=106153 AND spell=210304);
SELECT 'terrain',COUNT(*) FROM phase_definitions WHERE zoneId=7503 AND entry=7 AND PreloadMapID=1532;
SELECT 'terrain-active',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7503 AND SourceEntry=7 AND ElseGroup=1 AND ConditionTypeOrReference=9 AND ConditionValue1=42088 AND NegativeCondition=0;
SELECT 'next',COUNT(*) FROM quest_template_addon WHERE ID=42088 AND NextQuestID=42512;
"@
$sql+="SELECT 'hotfix-dialogue',COUNT(*) FROM creature_text t JOIN $hotfix.broadcast_text b ON t.BroadcastTextID=b.ID WHERE t.CreatureID IN(106115,106153,106191) AND t.GroupID=0 AND t.ID=0 AND b.VerifiedBuild=26124 AND BINARY t.Text=BINARY IF(t.CreatureID=106115,b.Text1,b.Text);"
$expected=@("ai`t3","obsolete-actions`t0","texts`t3","options`t3","active`t3","uncredited`t3","objective-shape`t3","spawns`t3","spawn-phase`t3","mayla-start`t1","mayla-end`t1","nefu-aura`t1","carry-spells`t2","terrain`t1","terrain-active`t1","next`t1","hotfix-dialogue`t3")
if(!$Live){
 $sql+=@"
SELECT 'other-ai',COUNT(*) FROM saved_creature_template a JOIN creature_template b USING(entry) WHERE a.entry NOT IN(106115,106153,106191) AND (a.AIName<>b.AIName OR a.ScriptName<>b.ScriptName);
SELECT 'other-text',COUNT(*) FROM saved_creature_text a LEFT JOIN creature_text b ON a.CreatureID=b.CreatureID AND a.GroupID=b.GroupID AND a.ID=b.ID AND BINARY a.Text=BINARY b.Text AND a.BroadcastTextID=b.BroadcastTextID WHERE a.CreatureID NOT IN(106115,106153,106191) AND b.CreatureID IS NULL;
SELECT 'other-options',COUNT(*) FROM saved_gossip_menu_option a JOIN gossip_menu_option b USING(MenuID,OptionID) WHERE a.MenuID NOT IN(19560,19561,19565) AND a.OptionText<>b.OptionText;
SELECT 'other-actions',(SELECT COUNT(*) FROM saved_smart_scripts WHERE entryorguid NOT IN(106115,106153,106191,10611500,10615300,10619100))-(SELECT COUNT(*) FROM smart_scripts WHERE entryorguid NOT IN(106115,106153,106191,10611500,10615300,10619100));
SELECT 'other-conditions',(SELECT COUNT(*) FROM saved_conditions WHERE SourceTypeOrReferenceId<>15 OR SourceGroup NOT IN(19560,19561,19565))-(SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId<>15 OR SourceGroup NOT IN(19560,19561,19565));
UPDATE creature_template SET ScriptName='custom_evacuation' WHERE entry=106115;
"@
 $sql+="`n$migration`nSELECT 'custom-ai-preserved',COUNT(*) FROM creature_template WHERE entry=106115 AND ScriptName='custom_evacuation';"
 $expected+=@("other-ai`t0","other-text`t0","other-options`t0","other-actions`t0","other-conditions`t0","custom-ai-preserved`t1")
}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $actual=$sql | & 'C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe' --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --skip-column-names
 if($LASTEXITCODE){throw 'Evacuation SQL test failed'}
} finally {$env:MYSQL_PWD=$previous}
if(($actual -join '|') -ne ($expected -join '|')){throw "Evacuation SQL mismatch: $($actual -join '|')"}
if($Live){'PASS: deployed evacuation scripts, conditions, texts, objectives, spawns, terrain and next quest.'}
else{'PASS: migration twice, native quest data, dialogue gating and unrelated/custom content preservation.'}
