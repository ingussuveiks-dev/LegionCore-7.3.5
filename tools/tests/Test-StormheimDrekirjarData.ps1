$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-drekirjar"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Read-Drekirjar([string]$database,[string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match "^${database}DatabaseInfo\s*="} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Drekirjar data query failed'}}finally{$env:MYSQL_PWD=$previous}
 @($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")})
}
$migration=Get-Content "$repo/sql/updates/world/2026_10_17_21_stormheim_drekirjar.sql" -Raw
$test=Read-Drekirjar World @"
CREATE TEMPORARY TABLE drekirjar_original LIKE gameobject_template;
INSERT INTO drekirjar_original SELECT * FROM gameobject_template WHERE entry IN(241429,245667);
CREATE TEMPORARY TABLE gameobject_template LIKE drekirjar_original;
INSERT INTO gameobject_template SELECT * FROM drekirjar_original;
UPDATE gameobject_template SET flags=1 WHERE entry=241429;
$migration
$migration
SELECT 'fixed',flags FROM gameobject_template WHERE entry=241429;
UPDATE gameobject_template SET flags=17 WHERE entry=241429;
$migration
SELECT 'other-bit',flags FROM gameobject_template WHERE entry=241429;
SELECT 'unrelated',COUNT(*) FROM gameobject_template t JOIN drekirjar_original o USING(entry) WHERE t.entry=245667 AND t.flags=o.flags AND t.Data10=o.Data10;
SELECT 'preserved',COUNT(*) FROM gameobject_template t JOIN drekirjar_original o USING(entry) WHERE t.entry=241429 AND t.Data10=o.Data10 AND t.Data5=o.Data5 AND t.Data3=o.Data3 AND t.ScriptName=o.ScriptName AND t.displayId=o.displayId AND t.WorldEffectID=o.WorldEffectID;
"@
$test | Set-Content "$out/migration.tsv"
foreach($r in @("fixed`t0","other-bit`t16","unrelated`t1","preserved`t1")){if($test -notcontains $r){throw "Drekirjar migration failed: $r"}}
$sql=@'
SELECT 'quest',q.ID,q.QuestType,q.Flags,q.FlagsEx,q.AllowableRaces,q.AreaGroupID,q.QuestInfoID,q.StartItem,q.QuestPackageID,a.PrevQuestID,a.NextQuestID,a.SpecialFlags,q.ItemDrop1,q.ItemDropQuantity1 FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=38374;
SELECT 'objective',QuestID,ID,Type,ObjectID,Amount,StorageIndex,Flags,TaskStep FROM quest_objectives WHERE QuestID=38374 ORDER BY ID;
SELECT 'source',w.Entry,w.KillCredit1,w.KillCredit2,t.faction,t.unit_flags,t.AIName,t.ScriptName,w.Displayid1,w.Displayid2,w.Displayid3,w.Displayid4,t.VehicleId,COUNT(c.guid),t.unit_flags2,t.unit_flags3,t.flags_extra FROM creature_template_wdb w JOIN creature_template t ON t.entry=w.Entry LEFT JOIN creature c ON c.id=w.Entry WHERE w.KillCredit1 IN(92238,92239,92240,92241,96992,100474) OR w.KillCredit2 IN(92238,92239,92240,92241,96992,100474) GROUP BY w.Entry ORDER BY w.Entry;
SELECT 'spawn',guid,id,map,zoneId,areaId,phaseMask,PhaseId,spawnMask,modelid,position_x,position_y,position_z FROM creature WHERE id IN(SELECT Entry FROM creature_template_wdb WHERE KillCredit1 IN(92238,92239,92240,92241,96992,100474) OR KillCredit2 IN(92238,92239,92240,92241,96992,100474)) ORDER BY id,guid;
SELECT 'scaling',Entry,LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry IN(SELECT Entry FROM creature_template_wdb WHERE KillCredit1 IN(92238,92239,92240,92241,96992,100474)) ORDER BY Entry;
SELECT 'go',entry,type,flags,Data0,Data1,Data2,Data3,Data4,Data5,Data6,Data7,Data10,Data12,Data19,Data22,Data23,AIName,ScriptName,displayId,WorldEffectID FROM gameobject_template WHERE entry IN(241429,245667,245668,245669,245670,245671,245672) ORDER BY entry;
SELECT 'go-spawn',guid,id,map,zoneId,areaId,phaseMask,PhaseId,spawnMask,spawntimesecs FROM gameobject WHERE id IN(241429,245667,245668,245669,245670,245671,245672) ORDER BY id,guid;
SELECT 'smart',entryorguid,source_type,id,link,event_type,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,target_type,target_param1,target_param2,target_x,target_y,target_z,Difficulties FROM smart_scripts WHERE(source_type=0 AND entryorguid IN(91892,91893,91894,91895)) OR(source_type=9 AND entryorguid IN(9189200,9189400,9189500,9281092)) ORDER BY source_type,entryorguid,id;
SELECT 'condition',SourceEntry,SourceGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry IN(91892,91893,91894,91895) ORDER BY SourceEntry,SourceGroup;
SELECT 'relations',COUNT(*) FROM(SELECT quest FROM creature_queststarter UNION ALL SELECT quest FROM creature_questender UNION ALL SELECT quest FROM gameobject_queststarter UNION ALL SELECT quest FROM gameobject_questender UNION ALL SELECT quest FROM area_queststart)r WHERE quest=38374;
SELECT 'quest-condition',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId IN(19,20) AND SourceEntry=38374;
SELECT 'smart-credit',COUNT(*) FROM smart_scripts WHERE action_type=33 AND action_param1 IN(92238,92239,92240,92241,96992,100474);
SELECT 'brew-overrides',COUNT(*) FROM smart_scripts WHERE source_type=1 AND(entryorguid=241429 OR entryorguid IN(SELECT -CAST(guid AS SIGNED) FROM gameobject WHERE id=241429));
SELECT 'brew-actions',COUNT(*) FROM gameobject_action WHERE entry=241429;
SELECT 'brew-spawn-scripts',COUNT(*) FROM gameobject_scripts WHERE id IN(SELECT guid FROM gameobject WHERE id=241429);
SELECT 'spell-scripts',COUNT(*) FROM spell_script_names WHERE spell_id IN(181858,181859,183149,183167,183180,183181,197885);
'@
$rows=Read-Drekirjar World $sql;$rows | Set-Content "$out/world.tsv"
$spawns=@($rows | Where-Object {$_ -like "spawn`t*"});if($spawns.Count -ne 156){throw 'Unexpected alias source spawn count'}
foreach($r in $spawns){$v=$r.Split("`t");$outdoor=$v[3] -eq '1220' -and $v[8] -eq '1';$scenario=$v[1] -eq '382219' -and $v[2] -eq '91487' -and $v[3] -eq '1495' -and $v[7] -eq '6383' -and $v[8] -eq '4096';if((!$outdoor -and !$scenario) -or $v[4] -ne '7541' -or $v[6] -ne '1' -or $v[9] -ne '0'){throw "Source map/model override changed: $r"}}
foreach($r in @($rows | Where-Object {$_ -like "source`t*"})){$v=$r.Split("`t");if([int]$v[13] -and $rows -notcontains "scaling`t$($v[1])`t98`t110"){throw 'Invalid/missing active source scaling'}}
if($rows -notcontains "quest`t38374`t3`t38338560`t8192`t18446744073709551615`t4410`t0`t0`t0`t0`t0`t0`t122612`t99"){throw 'Drekirjar task contract changed'}
$objectives=@("277754`t15`t0`t1`t-1`t0`t0","277999`t0`t92238`t100`t17`t92`t1","278000`t0`t92239`t50`t18`t92`t2","278001`t0`t92240`t34`t19`t92`t3","278002`t0`t92241`t20`t20`t92`t5","279892`t0`t96992`t10`t0`t92`t10","280858`t0`t100474`t7`t1`t92`t15")
foreach($r in $objectives){if($rows -notcontains "objective`t38374`t$r"){throw "Drekirjar objective changed: $r"}}
if(@($rows | Where-Object {$_ -like "objective`t*"}).Count -ne 7){throw 'Unexpected objective set'}
foreach($kind in @('relations','quest-condition','smart-credit','brew-overrides','brew-actions','brew-spawn-scripts','spell-scripts')){if($rows -notcontains "$kind`t0"){throw "Unexpected $kind"}}
$go=@($rows | Where-Object {$_ -like "go`t*"});if($go.Count -ne 7){throw 'Expected brew and six cage templates'}
foreach($r in $go){$v=$r.Split("`t");if($v[1] -eq '241429'){if($r -ne "go`t241429`t10`t0`t0`t0`t0`t3000`t0`t1`t0`t0`t183181`t0`t0`t0`t1`t`t`t23212`t2100"){throw 'Brew contract changed'}}else{if($v[2] -ne '10' -or $v[4] -ne '1691' -or $v[5] -ne '38374' -or $v[7] -ne '3000' -or $v[9] -ne '1' -or $v[12] -ne '197885' -or $v[17] -or $v[18] -or [int]$v[3] -band 17){throw "Cage contract changed: $r"}}}
$goSpawns=@($rows | Where-Object {$_ -like "go-spawn`t*"});if($goSpawns.Count -ne 12){throw 'Expected six brew and six cage spawns'}
foreach($r in $goSpawns){$v=$r.Split("`t");if($v[3] -ne '1220' -or $v[4] -ne '7541' -or $v[6] -ne '1' -or $v[7] -or $v[8] -ne '1' -or $v[9] -ne '180'){throw "GO spawn changed: $r"}}
foreach($entry in @(91893,91894,91895)){$group=@{91893=3;91894=4;91895=5}[$entry];if($rows -notcontains "condition`t$entry`t$group`t36`t1`t0`t1"){throw 'Elite death must evaluate the dying base actor'}}
$smart=@($rows | Where-Object {$_ -like "smart`t*"} | ForEach-Object {,$_.Split("`t")})
foreach($death in @(@(91893,2,1,91895),@(91895,4,2,91894),@(91894,3,3,91892))){$s=@($smart | Where-Object {$_[1] -eq $death[0] -and $_[2] -eq '0' -and $_[3] -eq $death[1]});if($s.Count -ne 1){throw 'Elite death event missing'};$v=$s[0];if($v[5] -ne '6' -or $v[6] -ne '1' -or $v[11] -ne '45' -or $v[12] -ne '0' -or $v[13] -ne $death[2] -or $v[14] -ne '11' -or $v[15] -ne $death[3] -or $v[16] -ne '50'){throw 'Elite SetData handoff changed'}}
foreach($list in @(9189500,9189400,9189200)){if(@($smart | Where-Object {$_[1] -eq $list -and $_[2] -eq '9' -and $_[11] -eq '19' -and $_[12] -eq '33536' -and $_[14] -eq '1'}).Count -ne 1){throw 'Elite timed unlock missing'}}
$displays=@($rows | Where-Object {$_ -like "source`t*"} | ForEach-Object {$v=$_.Split("`t");if([int]$v[13]){$v[8..11] | ForEach-Object {if([int]$_){[int]$_}}}} | Sort-Object -Unique)
$displayList=$displays -join ','
if((Read-Drekirjar World "SELECT COUNT(*) FROM creature_model_info WHERE DisplayID IN($displayList);") -notcontains "$($displays.Count)"){throw 'Missing World model metadata'}
$hotfix=Read-Drekirjar Hotfix "SELECT 'task',COUNT(*) FROM quest_v2_cli_task WHERE ID=38374; SELECT 'group',COUNT(*) FROM area_group_member WHERE AreaGroupID=4410; SELECT 'condition33048',COUNT(*) FROM player_condition WHERE ID=33048; SELECT 'effects',COUNT(*) FROM spell_effect WHERE SpellID IN(181858,181859,183149,183167,183180,183181,197885); SELECT 'item',COUNT(*) FROM item_sparse WHERE ID=122612; SELECT 'item-effect',COUNT(*) FROM item_effect WHERE ItemID=122612; SELECT 'display',COUNT(*) FROM creature_display_info WHERE ID IN($displayList); SELECT 'model',COUNT(*) FROM creature_model_data; SELECT 'faction',COUNT(*) FROM faction_template WHERE ID IN(14,16);"
$hotfix | Set-Content "$out/hotfix.tsv";foreach($kind in @('task','group','condition33048','effects','item','item-effect','display','model','faction')){if($hotfix -notcontains "$kind`t0"){throw "Unexpected native $kind override"}}
'PASS:38374 task/seven objectives, original source catalog and elite base-actor conditions, brew/cage spell links and migration isolation. Native activation, field packing and interaction tests remain separate.'
