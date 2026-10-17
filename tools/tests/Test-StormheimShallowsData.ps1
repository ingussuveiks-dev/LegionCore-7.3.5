param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-shallows"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Read-Shallows([string]$database,[string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match "^${database}DatabaseInfo\s*="} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Shallows data query failed'}}finally{$env:MYSQL_PWD=$previous}
 @($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")})
}
$migration=Get-Content "$repo/sql/updates/world/2026_10_17_18_stormheim_shallows.sql" -Raw
$fixture=@"
CREATE TEMPORARY TABLE saved_gameobject_template LIKE gameobject_template;
INSERT INTO saved_gameobject_template SELECT * FROM gameobject_template WHERE entry IN(252239,249742);
CREATE TEMPORARY TABLE gameobject_template LIKE saved_gameobject_template;
INSERT INTO gameobject_template SELECT * FROM saved_gameobject_template;
UPDATE gameobject_template SET Data1=0 WHERE entry=252239;
SELECT 'broken',Data1 FROM gameobject_template WHERE entry=252239;
$migration
$migration
SELECT 'fixed',Data1 FROM gameobject_template WHERE entry=252239;
SELECT 'preserved',COUNT(*) FROM gameobject_template g JOIN saved_gameobject_template s USING(entry) WHERE g.entry=252239 AND g.type=s.type AND g.flags=s.flags AND g.displayId=s.displayId AND g.size=s.size AND g.Data0=s.Data0 AND g.Data2=s.Data2 AND g.Data3=s.Data3 AND g.Data8=s.Data8 AND g.Data15=s.Data15 AND g.Data17=s.Data17 AND g.Data26=s.Data26 AND g.Data30=s.Data30 AND g.questItem1=s.questItem1 AND g.AIName=s.AIName AND g.ScriptName=s.ScriptName;
SELECT 'unrelated',COUNT(*) FROM gameobject_template g JOIN saved_gameobject_template s USING(entry) WHERE g.entry=249742 AND g.Data1=s.Data1;
UPDATE gameobject_template SET Data1=12345 WHERE entry=252239;
$migration
SELECT 'custom',Data1 FROM gameobject_template WHERE entry=252239;
UPDATE gameobject_template SET type=10,Data1=0 WHERE entry=252239;
$migration
SELECT 'other-type',Data1 FROM gameobject_template WHERE entry=252239;
"@
$rows=Read-Shallows World $fixture
foreach($line in @("broken`t0","fixed`t252239","preserved`t1","unrelated`t1","custom`t12345","other-type`t0")){if($rows -notcontains $line){throw "Shallows migration regression: $line"}}
$rows | Set-Content "$out/migration.tsv"
$setup=''
if(!$Live){$setup="CREATE TEMPORARY TABLE saved_go LIKE gameobject_template; INSERT INTO saved_go SELECT * FROM gameobject_template WHERE entry=252239; CREATE TEMPORARY TABLE gameobject_template LIKE saved_go; INSERT INTO gameobject_template SELECT * FROM saved_go; UPDATE gameobject_template SET Data1=0 WHERE entry=252239;`n$migration"}
$sql=@'
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,a.ProvidedItemCount,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=40046;
SELECT 'objective',QuestID,ID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID=40046 ORDER BY StorageIndex;
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest=40046;
SELECT 'npc-ender',id,quest FROM creature_questender WHERE quest=40046;
SELECT 'go-relation',id,quest FROM gameobject_queststarter WHERE quest=40046 UNION ALL SELECT 'go-relation',id,quest FROM gameobject_questender WHERE quest=40046;
SELECT 'chest',entry,type,displayId,flags,size,Data0,Data1,Data8,Data15,Data17,Data26,Data30,AIName,ScriptName FROM gameobject_template WHERE entry=252239;
SELECT 'loot',Entry,Item,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount,Reference,Currency FROM gameobject_loot_template WHERE Entry=252239 ORDER BY Item;
SELECT 'spawn',guid,id,map,position_x,position_y,position_z,orientation,phaseMask,PhaseId,spawnMask FROM gameobject WHERE id=252239 ORDER BY guid;
SELECT 'giver',entry,npcflag,faction,AIName,ScriptName FROM creature_template WHERE entry=97973;
SELECT 'havi',guid,map,position_x,position_y,position_z,PhaseId FROM creature WHERE id=97973;
SELECT 'extra-conditions',COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId IN(19,20) AND SourceEntry=40046) OR (SourceTypeOrReferenceId=1 AND (SourceGroup=252239 OR SourceEntry IN(129174,139542,139543,139544,139545))) OR (SourceTypeOrReferenceId IN(13,17) AND SourceEntry=227818);
SELECT 'object-smart',COUNT(*) FROM smart_scripts WHERE (entryorguid=252239 OR entryorguid IN(SELECT -CAST(guid AS SIGNED) FROM gameobject WHERE id=252239)) AND source_type=1;
SELECT 'spell-bindings',COUNT(*) FROM spell_script_names WHERE spell_id=227818;
SELECT 'spell-dummy',COUNT(*) FROM spell_dummy_trigger WHERE spell_id=227818;
SELECT 'spell-linked',COUNT(*) FROM spell_linked_spell WHERE spell_trigger IN(227818,-227818);
SELECT 'item-overrides',COUNT(*) FROM item_template WHERE entry=129174;
SELECT 'item-addons',COUNT(*) FROM item_template_addon WHERE Id=129174;
SELECT 'credit-objective',COUNT(*) FROM quest_objectives WHERE QuestID=40046 AND ObjectID=114487;
'@
$rows=Read-Shallows World ($setup+"`n"+$sql)
$rows | Set-Content "$out/world.tsv"
$expected=@("quest`t40046`t37748744`t0`t18446744073709551615`t0`t0`t0`t0`t0`t0","objective`t40046`t280209`t1`t129174`t1`t0`t0","objective`t40046`t285515`t2`t252239`t99`t1`t28","starter`t97973`t40046","npc-ender`t97973`t40046","chest`t252239`t3`t35915`t2113540`t0.5`t1691`t252239`t40046`t0`t0`t227818`t0`t`t","giver`t97973`t3`t35`tSmartAI`t","havi`t369621`t1220`t3521.58`t1615.26`t11.3535`t")
foreach($line in $expected){if($rows -notcontains $line){throw "Shallows contract changed: $line"}}
if(@($rows | Where-Object {$_ -like 'objective*'}).Count -ne 2 -or @($rows | Where-Object {$_ -like 'starter*'}).Count -ne 1 -or @($rows | Where-Object {$_ -like 'npc-ender*'}).Count -ne 1 -or @($rows | Where-Object {$_ -like 'go-relation*'}).Count){throw 'Shallows objective/questgiver set changed'}
$drops=@{129174='29.6569';139542='15.8502';139543='28.2908';139544='19.0264';139545='19.5843'}
if(@($rows | Where-Object {$_ -like "loot`t*"}).Count -ne 5){throw 'Shallows loot set changed'}
foreach($id in $drops.Keys){$required=[int]($id -eq 129174);if($rows -notcontains "loot`t252239`t$id`t$($drops[$id])`t$required`t1`t0`t1`t1`t0`t0"){throw "Shallows drop changed: $id"}}
$spawns=@($rows | Where-Object {$_ -like "spawn`t*"});if($spawns.Count -ne 19){throw 'Expected19 debris spawns'}
foreach($line in $spawns){$v=$line.Split("`t");if($v[2] -ne '252239' -or $v[3] -ne '1220' -or $v[8] -ne '1' -or $v[9] -ne '' -or $v[10] -ne '1'){throw "Unexpected debris map/phase/difficulty: $line"}}
foreach($kind in @('extra-conditions','object-smart','spell-bindings','spell-dummy','spell-linked','item-overrides','item-addons','credit-objective')){if($rows -notcontains "$kind`t0"){throw "Unexpected $kind"}}
$rows=Read-Shallows Hotfix @'
SELECT 'spell',COUNT(*) FROM spell_effect WHERE SpellID IN(227818,3365); SELECT 'spell-misc',COUNT(*) FROM spell_misc WHERE SpellID IN(227818,3365); SELECT 'item',COUNT(*) FROM item_sparse WHERE ID=129174; SELECT 'uses',COUNT(*) FROM item_effect WHERE ItemID=129174; SELECT 'display',COUNT(*) FROM gameobject_display_info WHERE ID=35915; SELECT 'lock',COUNT(*) FROM `lock` WHERE ID=1691;
'@
$rows | Set-Content "$out/hotfix.tsv"
foreach($kind in @('spell','spell-misc','item','uses','display','lock')){if($rows -notcontains "$kind`t0"){throw "Native Shallows $kind overridden"}}
'PASS: missing selector reproduced, migration twice/custom/type/isolation checks,19 spawns, both quest relations, optional99 searches, five unchanged loot rows and no scoped overrides.'
