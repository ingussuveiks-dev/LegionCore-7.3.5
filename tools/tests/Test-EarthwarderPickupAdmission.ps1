param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot verify current Release'}
if($Baseline){& "$PSScriptRoot/Test-EarthwarderReturn.ps1" -PickupBaseline}else{& "$PSScriptRoot/Test-EarthwarderReturn.ps1" -PickupAdmission -VerifyReleaseBinary:$VerifyReleaseBinary}
if(!$? -or $LASTEXITCODE){throw 'Earthwarder pickup host failed'}
if($VerifyReleaseBinary){
 $repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
 $out="$repo/build-extractors/runtime-test/earthwarder-return"
 if((Get-Content "$out/native.tsv") -notcontains "EARTH_CRITERION`t5`t28550`t92`t45785`t0"){throw 'Native909 final criterion changed; do not invent a count1 requirement'}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$rows=@(@'
SELECT 'binding',entry,type,displayId,flags,Data0,Data1,Data14,AIName,ScriptName FROM gameobject_template WHERE ScriptName='go_art_earthwarder';
SELECT 'spawn',guid,id,map,PhaseId FROM gameobject WHERE id=248831;
SELECT 'loot',entry,item,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount FROM gameobject_loot_template WHERE entry=248831;
SELECT 'phase',ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=3;
SELECT 'smart',COUNT(*) FROM smart_scripts WHERE source_type=1 AND entryorguid IN(248831,-134175);
SELECT 'scenario',ScenarioID,MapID,DifficultyID,Team,Class,LfgDungeonID FROM scenario_data WHERE MapID=1495 ORDER BY ScenarioID;
'@|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Native Earthwarder binding/phase read failed'}}finally{$env:MYSQL_PWD=$previous}
 [IO.File]::WriteAllLines("$out/pickup-binding.tsv",[string[]]$rows)
 $expected=@("binding`t248831`t3`t9806`t2113536`t1691`t248831`t107711`t`tgo_art_earthwarder","spawn`t134175`t248831`t1495`t6363 5048 7000","loot`t248831`t128289`t100`t0`t1`t0`t1`t1","phase`t44`t0`t909`t5`t0`t0","smart`t0","scenario`t909`t1495`t0`t0`t1`t0","scenario`t1068`t1495`t0`t0`t3`t0","scenario`t1082`t1495`t0`t0`t2`t0")
 if(Compare-Object $expected $rows){throw "Native Earthwarder binding differs; inspect $out/pickup-binding.tsv"}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_earthwarder_pickup_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Earthwarder admission missing'}}finally{$env:PYTHONPATH=$previous}
}
