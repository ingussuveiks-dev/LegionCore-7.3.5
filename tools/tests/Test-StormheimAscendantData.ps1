param([switch]$Live,[switch]$BothFactions)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-ascendant"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_17_09_stormheim_ascendant_communion.sql" -Raw
$sql=''
if(!$Live){
 foreach($t in @('creature_template','conditions','smart_scripts','spell_script_names')){
  $sql+="CREATE TEMPORARY TABLE saved_$t LIKE $t;`n"
  if($t -eq 'creature_template'){$sql+="INSERT INTO saved_$t SELECT * FROM $t WHERE entry IN(94393,95804); UPDATE saved_$t SET ScriptName='' WHERE entry=94393;`n"}
  $sql+="CREATE TEMPORARY TABLE $t LIKE saved_$t;`n"
 }
 $sql+=@'
INSERT INTO creature_template SELECT * FROM saved_creature_template;
INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,event_param1,action_type,action_param1,target_type,comment) VALUES(94393,0,0,62,18543,33,94477,7,'original'),(94393,0,1,62,18543,72,0,7,'close');
INSERT INTO spell_script_names VALUES(186982,'custom_communion'),(999,'unrelated');
'@
 $sql+=$migration+"`n"+$migration+"`n"
}
if($BothFactions -and !$Live){
 $hordeMigration=Get-Content "$repo/sql/updates/world/2026_10_17_12_stormheim_horde_communion.sql" -Raw
 $sql+="SELECT 'old-condition',ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ElseGroup FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18543 AND SourceEntry=0;`n"+$hordeMigration+"`n"+$hordeMigration+"`n"
}
$sql+=@'
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.ExclusiveGroup,a.SpecialFlags,q.QuestPackageID FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID IN(39092,39122) ORDER BY q.ID;
SELECT 'objective',QuestID,ID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID=39092 ORDER BY StorageIndex;
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest IN(39092,39122) ORDER BY quest;
SELECT 'ender',id,quest FROM creature_questender WHERE quest=39092;
SELECT 'go',entry,type,displayId,flags,size,Data0,Data1,Data2,Data3,Data7,Data10,Data19,Data23,ScriptName FROM gameobject_template WHERE entry IN(242994,242995,242996,242998) ORDER BY entry;
SELECT 'go-position',guid,id,map,position_x,position_y,position_z,PhaseId FROM gameobject WHERE id IN(242994,242995,242996,242998) ORDER BY id;
SELECT 'npc-position',guid,id,map,position_x,position_y,position_z,PhaseId FROM creature WHERE id IN(94393,95804,95921) ORDER BY id;
SELECT 'statue',t.entry,t.gossip_menu_id,t.npcflag,t.AIName,t.ScriptName,w.Displayid1 FROM creature_template t JOIN creature_template_wdb w ON w.Entry=t.entry WHERE t.entry=94393;
SELECT 'condition',ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ElseGroup FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18543 AND SourceEntry=0 ORDER BY ConditionTypeOrReference,ConditionValue2;
SELECT 'legacy',COUNT(*) FROM smart_scripts WHERE entryorguid=94393 AND source_type=0 AND action_type=33 AND action_param1=94477;
SELECT 'binding',COUNT(*) FROM spell_script_names WHERE spell_id=186982 AND ScriptName='spell_stormheim_communing';
SELECT 'menu',MenuID,TextID FROM gossip_menu WHERE MenuID=18543;
SELECT 'option',MenuID,OptionID,OptionNpc,OptionBroadcastTextID FROM gossip_menu_option WHERE MenuID=18543;
SELECT 'spellarea',spell,area,quest_start,quest_start_status FROM spell_area WHERE quest_start=39092;
'@
if($BothFactions){
 $sql=$sql.Replace('q.ID IN(39092,39122)','q.ID IN(39092,39122,39155,38882)').Replace('WHERE QuestID=39092 ORDER BY','WHERE QuestID IN(39092,39155) ORDER BY').Replace('quest IN(39092,39122)','quest IN(39092,39122,39155,38882)').Replace('quest=39092;','quest IN(39092,39155);').Replace('id IN(94393,95804,95921)','id IN(94393,95804,95921,95611,94228)')
}
if($BothFactions -and !$Live){$sql+=@"
DELETE FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18543 AND SourceEntry=0 AND ElseGroup=1;
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionValue1,Comment) VALUES(15,18543,0,2,9,39155,'custom');
$hordeMigration
SELECT 'horde-custom-guard',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18543 AND SourceEntry=0 AND ElseGroup=1;
"@}
if(!$Live){$sql+=@"
SELECT 'preserved-binding',COUNT(*) FROM spell_script_names WHERE ScriptName IN('custom_communion','unrelated');
SELECT 'unrelated',COUNT(*) FROM creature_template c JOIN saved_creature_template s ON s.entry=c.entry WHERE c.entry=95804 AND c.ScriptName=s.ScriptName;
DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_creature_template;
DELETE FROM smart_scripts; INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,event_param1,action_type,action_param1,target_type,comment) VALUES(94393,0,0,62,18543,33,94477,7,'original'),(94393,0,7,1,0,72,0,1,'custom');
DELETE FROM conditions;
$migration
SELECT 'custom-script',ScriptName='' FROM creature_template WHERE entry=94393;
SELECT 'custom-legacy',COUNT(*) FROM smart_scripts WHERE entryorguid=94393;
SELECT 'custom-conditions',COUNT(*) FROM conditions;
"@}
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Ascendant World query/replay failed'}}finally{$env:MYSQL_PWD=$old}
$rows=@($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$rows | Set-Content "$out/world$(if($Live){'-live'}).tsv"
foreach($r in @("quest`t39092`t35651592`t262144`t824181837`t0`t39122`t39063`t39122`t0`t0`t0","objective`t39092`t278939`t2`t242994`t1`t0`t0","objective`t39092`t278940`t2`t242995`t1`t1`t0","objective`t39092`t278941`t2`t242996`t1`t2`t0","objective`t39092`t278942`t2`t242998`t1`t3`t0","objective`t39092`t278973`t0`t94477`t1`t4`t2","starter`t95921`t39092","starter`t95804`t39122","ender`t95804`t39092","menu`t18543`t26821","option`t18543`t0`t0`t96207","legacy`t0","binding`t1","condition`t9`t0`t39092`t0`t0`t0`t0")){if($rows -notcontains $r){throw "Ascendant contract changed: $r"}}
foreach($id in @(242994,242995,242996,242998)){if($rows -notcontains "condition`t41`t0`t39092`t$id`t1`t0`t0"){throw 'Missing actual shrine-objective menu condition'}}
foreach($r in @(
 "go`t242994`t10`t26892`t262144`t4`t43`t0`t0`t3000`t0`t186965`t0`t1`t",
 "go`t242995`t10`t27227`t262144`t3`t43`t0`t0`t3000`t0`t187780`t0`t1`t",
 "go`t242996`t10`t35263`t262144`t1.5`t43`t0`t0`t3000`t0`t186964`t0`t1`t",
 "go`t242998`t10`t35264`t262144`t1.5`t43`t0`t0`t3000`t0`t186972`t0`t1`t",
 "statue`t94393`t18543`t1`tSmartAI`tnpc_stormheim_ascendant_statue`t70980",
 "spellarea`t100044`t7736`t39092`t74"
)){if($rows -notcontains $r){throw "Native shrine/statue contract changed: $r"}}
$conditionCount=@($rows | Where-Object {$_ -like "condition`t*"}).Count
if($conditionCount -ne 5 -and $conditionCount -ne 10){throw 'Unexpected communion option conditions'}
if($BothFactions -and $conditionCount -ne 10){throw 'Shared statue missing Horde conditions'}
if($BothFactions){foreach($r in @("quest`t39155`t35651592`t262144`t234881970`t0`t38882`t38878`t38882`t0`t0`t0","objective`t39155`t279018`t2`t242994`t1`t0`t0","objective`t39155`t279019`t2`t242995`t1`t1`t0","objective`t39155`t279020`t2`t242996`t1`t2`t0","objective`t39155`t279021`t2`t242998`t1`t3`t0","objective`t39155`t279022`t0`t94477`t1`t4`t2","starter`t95611`t39155","starter`t94228`t38882","ender`t94228`t39155")){if($rows -notcontains $r){throw "Horde communion contract changed: $r"}}}
if($conditionCount -eq 10){
 foreach($r in @("condition`t9`t0`t39155`t0`t0`t0`t1")+(@(242994,242995,242996,242998) | ForEach-Object {"condition`t41`t0`t39155`t$_`t1`t0`t1"})){if($rows -notcontains $r){throw "Invalid Horde OR branch: $r"}}
}
if($BothFactions -and !$Live -and $rows -notcontains "horde-custom-guard`t0"){throw 'Horde conditions replaced custom menu logic'}
if(!$Live){foreach($r in @("preserved-binding`t2","unrelated`t1","custom-script`t1","custom-legacy`t2","custom-conditions`t0")){if($rows -notcontains $r){throw 'Communion migration overwrote custom/unrelated data'}}}
$sql="SELECT 'spell',COUNT(*) FROM spell_effect WHERE SpellID IN(186982,186965,187780,186964,186972); SELECT 'misc',COUNT(*) FROM spell_misc WHERE SpellID IN(186982,186965,187780,186964,186972); SELECT 'visual',COUNT(*) FROM spell_x_spell_visual WHERE SpellID IN(186982,186965,187780,186964,186972); SELECT 'lock',COUNT(*) FROM ``lock`` WHERE ID=43; SELECT 'text',ID,LENGTH(Text)+LENGTH(Text1) FROM broadcast_text WHERE ID IN(96206,96207,96136);"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^HotfixDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
try{$env:MYSQL_PWD=$c[3];$h=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Ascendant native override/text query failed'}}finally{$env:MYSQL_PWD=$old}
$h=@($h | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")});$h | Set-Content "$out/hotfix.tsv"
foreach($kind in @('spell','misc','visual','lock')){if($h -notcontains "$kind`t0"){throw "Unexpected Ascendant native $kind override"}}
'PASS: four shrine GO goals and sequenced personal final goal, native communion binding, option admission and isolated repeat/custom preservation; native spells remain unchanged.'
