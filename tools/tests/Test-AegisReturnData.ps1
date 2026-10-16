$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/aegis-return"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$sql=@"
SELECT 'quest',q.ID,q.Flags,q.AllowableRaces,q.StartItem,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=43349;
SELECT 'objective',ID,QuestID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID=43349;
SELECT 'starter',id,quest FROM gameobject_queststarter WHERE quest=43349;
SELECT 'ender',id,quest FROM gameobject_questender WHERE quest=43349;
SELECT 'go',entry,type,displayId,flags,size,Data0,Data1,Data2,AIName,ScriptName,SpellStateVisualID FROM gameobject_template WHERE entry=251992;
SELECT 'spawn',guid,id,map,zoneId,areaId,spawnMask,phaseMask,PhaseId,position_x,position_y,position_z FROM gameobject WHERE id=251992;
SELECT 'event',guid,id,map,zoneId,areaId,PhaseId,position_x,position_y,position_z FROM eventobject WHERE id=510;
SELECT 'event-template',entry,radius,SpellID,WorldSafeLocID,Flags,ScriptName FROM eventobject_template WHERE entry=510;
SELECT 'smart',id,link,event_type,action_type,action_param1,target_type,Difficulties,event_flags FROM smart_scripts WHERE entryorguid=510 AND source_type=13 ORDER BY id;
SELECT 'override-count',COUNT(*) FROM smart_scripts WHERE entryorguid=-410008 AND source_type=13;
SELECT 'phase',entry,phaseId,flags FROM phase_definitions WHERE zoneId=7502 AND entry IN(216,217) ORDER BY entry;
SELECT 'phase-condition',SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionTarget,ConditionValue1,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=23 AND ((SourceGroup=7502 AND SourceEntry IN(216,217)) OR (SourceGroup=7672 AND SourceEntry=7)) ORDER BY SourceGroup,SourceEntry,ConditionValue1;
SELECT 'event-condition-count',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId=22 AND SourceEntry IN(510,-410008)) OR (SourceTypeOrReferenceId=13 AND SourceEntry=510);
SELECT 'menu',MenuID,OptionID,OptionNpc FROM gossip_menu_option WHERE MenuID=20353;
SELECT 'next-count',COUNT(*) FROM quest_template_addon WHERE PrevQuestID=43349;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Aegis return query failed'}}finally{$env:MYSQL_PWD=$previous}
$expected=@(
 "quest`t43349`t33554440`t18446744073709551615`t0`t0`t40072`t0`t0",
 "objective`t285226`t43349`t0`t110409`t1`t0`t0",
 "starter`t251991`t43349","ender`t251992`t43349",
 "go`t251992`t2`t35704`t0`t0.75`t0`t20353`t0`t`t`t37794",
 "spawn`t25178660`t251992`t1220`t7502`t8045`t1`t1`t4592`t-854.191`t4456.02`t592.614",
 "event`t410008`t510`t1220`t7502`t8045`t`t-785.162`t4419.39`t602.464",
 "event-template`t510`t5`t0`t0`t0`tSmartEventObject",
 "smart`t0`t1`t89`t33`t107587`t7`t`t0","smart`t1`t2`t61`t33`t110409`t7`t`t0",
 "smart`t2`t3`t61`t33`t109750`t7`t`t0","smart`t3`t0`t61`t33`t106815`t7`t`t0",
 "override-count`t0","phase`t216`t4592`t16","phase`t217`t4591`t16",
 "phase-condition`t7502`t216`t8`t0`t43349`t1","phase-condition`t7502`t217`t8`t0`t43349`t0",
 "phase-condition`t7672`t7`t14`t0`t40072`t1","phase-condition`t7672`t7`t14`t0`t43349`t0",
 "event-condition-count`t0","menu`t20353`t0`t28","next-count`t0")
if($rows.Count -ne $expected.Count -or (Compare-Object $rows $expected)){throw 'Aegis return live contract differs; inspect query before changing progression'}
$rows | Set-Content "$out/world.tsv"
$sql="SELECT 'go-display',COUNT(*) FROM gameobject_display_info WHERE ID=35704; SELECT 'phase',COUNT(*) FROM phase WHERE ID IN(3554,4591,4592); SELECT 'spell-effect',COUNT(*) FROM spell_effect WHERE Effect IN(90,134) AND EffectMiscValue1=110409;"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Aegis return Hotfix query failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($name in @('go-display','phase','spell-effect')){if($rows -notcontains "$name`t0"){throw "Unexpected native-reference overlay $name"}}
$rows | Set-Content "$out/hotfix.tsv"
'PASS: existing43349 starter/ender, portal entry credit LINK chain, no DB override, native phase gates and unchanged terminal quest.'
