$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-tideskorn"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Read-Tideskorn([string]$database,[string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match "^${database}DatabaseInfo\s*="} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Tideskorn data query failed'}}finally{$env:MYSQL_PWD=$previous}
 @($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")})
}
$migration=Get-Content "$repo/sql/updates/world/2026_10_17_20_stormheim_tideskorn.sql" -Raw
$simulation=Read-Tideskorn World @"
CREATE TEMPORARY TABLE tideskorn_original LIKE gameobject_template;
INSERT INTO tideskorn_original SELECT * FROM gameobject_template WHERE entry IN(244703,244704,244729,244730,244731,244733,241741);
CREATE TEMPORARY TABLE gameobject_template LIKE tideskorn_original;
INSERT INTO gameobject_template SELECT * FROM tideskorn_original;
UPDATE gameobject_template SET flags=1 WHERE entry IN(244703,244704,244729,244730,244731,244733);
$migration
$migration
SELECT 'fixed',COUNT(*) FROM gameobject_template WHERE entry IN(244703,244704,244729,244730,244731,244733) AND flags=0;
UPDATE gameobject_template SET flags=17 WHERE entry=244703;
$migration
SELECT 'other-bit',flags FROM gameobject_template WHERE entry=244703;
SELECT 'unrelated',COUNT(*) FROM gameobject_template t JOIN tideskorn_original o USING(entry) WHERE t.entry=241741 AND t.flags=o.flags AND t.ScriptName=o.ScriptName;
SELECT 'other-fields',COUNT(*) FROM gameobject_template t JOIN tideskorn_original o USING(entry) WHERE t.entry IN(244703,244704,244729,244730,244731,244733) AND t.Data10=o.Data10 AND t.Data5=o.Data5 AND t.Data3=o.Data3 AND t.ScriptName=o.ScriptName AND t.displayId=o.displayId AND t.SpellVisualID=o.SpellVisualID;
"@
$simulation | Set-Content "$out/migration.tsv"
foreach($r in @("fixed`t6","other-bit`t16","unrelated`t1","other-fields`t6")){if($simulation -notcontains $r){throw "Tideskorn migration failed: $r"}}
$sql=@'
SELECT 'quest',q.ID,q.QuestType,q.Flags,q.FlagsEx,q.AllowableRaces,q.AreaGroupID,q.QuestInfoID,q.StartItem,q.QuestPackageID,a.PrevQuestID,a.NextQuestID,a.SpecialFlags FROM quest_template q JOIN quest_template_addon a ON a.ID=q.ID WHERE q.ID=39998;
SELECT 'objective',QuestID,ID,Type,ObjectID,Amount,StorageIndex,Flags,TaskStep FROM quest_objectives WHERE QuestID=39998 ORDER BY ID;
SELECT 'source',w.Entry,w.KillCredit1,w.KillCredit2,t.faction,t.unit_flags,t.AIName,t.ScriptName,COUNT(c.guid) FROM creature_template_wdb w JOIN creature_template t ON t.entry=w.Entry LEFT JOIN creature c ON c.id=w.Entry WHERE w.KillCredit1 BETWEEN 97945 AND 97949 OR w.KillCredit2 BETWEEN 97945 AND 97949 GROUP BY w.Entry ORDER BY w.Entry;
SELECT 'spawn',guid,id,map,zoneId,areaId,phaseMask,PhaseId,spawnMask,modelid FROM creature WHERE id IN(SELECT Entry FROM creature_template_wdb WHERE KillCredit1 BETWEEN 97945 AND 97949 OR KillCredit2 BETWEEN 97945 AND 97949) ORDER BY id,guid;
SELECT 'relations',COUNT(*) FROM(SELECT quest FROM creature_queststarter UNION ALL SELECT quest FROM creature_questender UNION ALL SELECT quest FROM gameobject_queststarter UNION ALL SELECT quest FROM gameobject_questender UNION ALL SELECT quest FROM area_queststart)r WHERE quest=39998;
SELECT 'conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId IN(19,20) AND SourceEntry=39998;
SELECT 'smart-credit',COUNT(*) FROM smart_scripts WHERE action_type=33 AND action_param1 BETWEEN 97945 AND 97949;
SELECT 'alias-spawns',COUNT(*) FROM creature WHERE id BETWEEN 97945 AND 97949;
SELECT 'scaling',Entry,LevelScalingMin,LevelScalingMax FROM creature_template_scaling WHERE Entry IN(SELECT Entry FROM creature_template_wdb WHERE KillCredit1 BETWEEN 97945 AND 97949) ORDER BY Entry;
SELECT 'go',entry,type,flags,Data0,Data1,Data2,Data3,Data4,Data5,Data6,Data7,Data10,Data12,Data19,Data22,Data23,ScriptName FROM gameobject_template WHERE Data10 IN(193190,193191) ORDER BY entry;
SELECT 'go-spawn',guid,id,map,zoneId,areaId,phaseMask,PhaseId,spawntimesecs,state FROM gameobject WHERE id IN(244703,244704,244729,244730,244731,244733) ORDER BY id,guid;
SELECT 'go-conditions',COUNT(*) FROM conditions WHERE(SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(193190,193191)) OR(SourceTypeOrReferenceId=22 AND SourceEntry IN(244703,244704,244729,244730,244731,244733));
SELECT 'spell-scripts',COUNT(*) FROM spell_script_names WHERE spell_id IN(193190,193191);
SELECT 'go-actions',COUNT(*) FROM gameobject_action WHERE entry IN(244703,244704,244729,244730,244731,244733);
SELECT 'go-smart',COUNT(*) FROM smart_scripts WHERE source_type=1 AND(entryorguid IN(244703,244704,244729,244730,244731,244733) OR entryorguid IN(SELECT -CAST(guid AS SIGNED) FROM gameobject WHERE id IN(244703,244704,244729,244730,244731,244733)));
'@
$rows=Read-Tideskorn World $sql;$rows | Set-Content "$out/world.tsv"
if($rows -notcontains "quest`t39998`t3`t38338560`t8192`t18446744073709551615`t4306`t0`t0`t0`t0`t0`t0"){throw 'Tideskorn task contract changed'}
$objectives=@("objective`t39998`t280149`t15`t0`t1`t-1`t0`t0","objective`t39998`t280155`t0`t97945`t200`t1`t92`t0.5","objective`t39998`t280156`t0`t97946`t100`t2`t92`t1","objective`t39998`t280157`t0`t97947`t50`t3`t92`t2","objective`t39998`t280158`t0`t97948`t34`t4`t92`t3","objective`t39998`t280159`t0`t97949`t20`t5`t92`t5")
if(@($rows | Where-Object {$_ -like "objective`t*"}).Count -ne 6){throw 'Unexpected objective set'}
foreach($r in $objectives){if($rows -notcontains $r){throw "Tideskorn objective changed: $r"}}
$expected=@{95620=@(97945,14);97816=@(97946,30);97821=@(97946,22);97822=@(97948,1);97825=@(97947,8);97851=@(97946,23);97859=@(97948,1);97906=@(97945,31);97942=@(97948,1);97944=@(97948,1);105083=@(97947,0);105084=@(97945,0);105127=@(97947,0);105129=@(97947,1);112479=@(97946,4);112480=@(97945,4);112481=@(97946,4);113171=@(97947,2)}
$sources=@($rows | Where-Object {$_ -like "source`t*"});if($sources.Count -ne $expected.Count){throw 'Unexpected credit source set'}
foreach($r in $sources){$v=$r.Split("`t");$id=[int]$v[1];$e=$expected[$id];if(!$e -or [int]$v[2] -ne $e[0] -or [int]$v[8] -ne $e[1] -or $v[6] -or $v[7]){throw "Credit source changed: $r"};if($e[1] -and ($v[3] -ne '105123' -or $v[4] -ne '14' -or $v[5] -ne '32768')){throw "Hostile source changed: $r"}}
$spawns=@($rows | Where-Object {$_ -like "spawn`t*"});if($spawns.Count -ne 147){throw "Unexpected spawn count: $($spawns.Count)"}
foreach($r in $spawns){$v=$r.Split("`t");$id=[int]$v[2];$area=if($id -in @(112479,112480,112481)){'7852'}else{'7643'};$phase=if($id -eq 105129){'6175'}else{''};if($v[3] -ne '1220' -or $v[4] -ne '7541' -or $v[5] -ne $area -or $v[6] -ne '1' -or $v[7] -ne $phase -or $v[8] -ne '1' -or $v[9] -ne '0'){throw "Spawn scope changed: $r"}}
foreach($kind in @('relations','conditions','smart-credit','alias-spawns','go-conditions','spell-scripts','go-actions','go-smart')){if($rows -notcontains "$kind`t0"){throw "Unexpected $kind"}}
$objects=@($rows | Where-Object {$_ -like "go`t*"});if($objects.Count -ne 6){throw 'Expected banner and five circle templates'}
foreach($r in $objects){$v=$r.Split("`t");$banner=$v[1] -eq '244704';$spell=if($banner){193190}else{193191};$playerCast=if($banner){1}else{0};$script=if($banner){'go_grey_shoals_supplies'}else{''};if($r -ne "go`t$($v[1])`t10`t0`t1691`t0`t0`t3000`t0`t1`t0`t0`t$spell`t0`t0`t36239`t$playerCast`t$script"){throw "GO contract changed: $r"}}
$goSpawns=@($rows | Where-Object {$_ -like "go-spawn`t*"});if($goSpawns.Count -ne 24){throw 'Expected24 banner/circle spawns'}
foreach($r in $goSpawns){$v=$r.Split("`t");if($v[3] -ne '1220' -or $v[6] -ne '1' -or $v[7] -ne '' -or $v[8] -ne '180' -or $v[9] -ne '1'){throw "GO spawn changed: $r"};if($v[5] -ne '7643' -and !($v[2] -eq '244704' -and $v[5] -eq '8297')){throw 'Unexpected GO area'}}
$hotfix=Read-Tideskorn Hotfix "SELECT 'task',COUNT(*) FROM quest_v2_cli_task WHERE ID=39998; SELECT 'group',COUNT(*) FROM area_group_member WHERE AreaGroupID=4306; SELECT 'effects',COUNT(*) FROM spell_effect WHERE Effect IN(90,134) AND EffectMiscValue1 BETWEEN 97945 AND 97949; SELECT 'condition36239',COUNT(*) FROM player_condition WHERE ID=36239;"
$hotfix | Set-Content "$out/hotfix.tsv";foreach($r in @("task`t0","group`t0","effects`t0","condition36239`t0")){if($hotfix -notcontains $r){throw 'Native task/group/credit data overridden'}}
'PASS: migration twice, unrelated bits/template fields preserved;39998 task/six weighted objectives,18 source templates/147 creature spawns, six GO templates/24 spawns; original2%/5% spell links and shared Grey Shoals script retained; no activation/credit override.'
