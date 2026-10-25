param([switch]$VerifyReleaseBinary,[switch]$Baseline)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot verify current Release'}
if($Baseline){& "$PSScriptRoot/Test-TruthguardPickupAdmission.ps1" -Baseline; if(!$? -or $LASTEXITCODE){throw 'Pinned callback baseline failed'}; return}
& "$PSScriptRoot/Test-TruthguardPickupAdmission.ps1"
if(!$? -or $LASTEXITCODE){throw 'Native loot lifecycle failed'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/truthguard-real-loot"
New-Item -ItemType Directory -Force $out|Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_25_07_truthguard_real_loot.sql" -Raw
$tables=@('gameobject_template','gameobject_loot_template','conditions','smart_scripts','gameobject')
$scope='SourceTypeOrReferenceId=4 AND SourceGroup=249420 AND SourceEntry=128866'
$reset=($tables|ForEach-Object {"DELETE FROM $_;INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$reset+="`nUPDATE gameobject_template SET Data1=0 WHERE entry=249420;DELETE FROM conditions WHERE $scope;"
$sql=@'
CREATE TEMPORARY TABLE saved_gameobject_template LIKE gameobject_template;
INSERT INTO saved_gameobject_template SELECT * FROM gameobject_template WHERE entry IN(249420,248834);
CREATE TEMPORARY TABLE saved_gameobject_loot_template LIKE gameobject_loot_template;
INSERT INTO saved_gameobject_loot_template SELECT * FROM gameobject_loot_template WHERE Entry IN(249420,248834);
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE (SourceTypeOrReferenceId=4 AND SourceGroup IN(249420,248834)) OR (SourceTypeOrReferenceId=23 AND SourceGroup=6383);
CREATE TEMPORARY TABLE saved_smart_scripts LIKE smart_scripts;
INSERT INTO saved_smart_scripts SELECT * FROM smart_scripts WHERE entryorguid IN(249420,-140818) AND source_type=1;
CREATE TEMPORARY TABLE saved_gameobject LIKE gameobject;
INSERT INTO saved_gameobject SELECT * FROM gameobject WHERE id IN(249420,248834);
'@
foreach($table in $tables){$sql+="`nCREATE TEMPORARY TABLE $table LIKE saved_$table;"}
$sql+="$reset`nSELECT 'before-template',current.* FROM gameobject_template current ORDER BY entry;`n$migration`nSELECT 'first-template',current.* FROM gameobject_template current ORDER BY entry;SELECT 'first-gates',current.* FROM conditions current ORDER BY 2,3,4,6,7;`n$migration`nSELECT 'second-template',current.* FROM gameobject_template current ORDER BY entry;SELECT 'second-gates',current.* FROM conditions current ORDER BY 2,3,4,6,7;`n"
foreach($table in @('gameobject_loot_template','smart_scripts','gameobject')){$sql+="SELECT '$table-before',current.* FROM saved_$table current ORDER BY 2,3;SELECT '$table-after',current.* FROM $table current ORDER BY 2,3;`n"}
$sql+="SELECT 'other-before',current.* FROM saved_conditions current WHERE NOT($scope) ORDER BY 2,3,4,6,7;SELECT 'other-after',current.* FROM conditions current WHERE NOT($scope) ORDER BY 2,3,4,6,7;SELECT 'gate',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope ORDER BY ElseGroup,ConditionTypeOrReference;SELECT 'link',Data1 FROM gameobject_template WHERE entry=249420;`n"
$variants=[ordered]@{
 'custom-script'="UPDATE gameobject_template SET ScriptName='custom' WHERE entry=249420;"
 'custom-ai'="UPDATE gameobject_template SET AIName='SmartGameObjectAI' WHERE entry=249420;"
 'custom-link'='UPDATE gameobject_template SET Data1=123 WHERE entry=249420;'
 'custom-personal'='UPDATE gameobject_template SET Data30=1 WHERE entry=249420;'
 'custom-security'='UPDATE gameobject_template SET Data31=1 WHERE entry=249420;'
 'custom-group'='UPDATE gameobject_template SET Data15=1 WHERE entry=249420;'
 'custom-player-condition'='UPDATE gameobject_template SET Data17=1 WHERE entry=249420;'
 'custom-chance'='UPDATE gameobject_loot_template SET Chance=50 WHERE Entry=249420;'
 'custom-reference'='UPDATE gameobject_loot_template SET Reference=1 WHERE Entry=249420;'
 'custom-count'='UPDATE gameobject_loot_template SET MaxCount=2 WHERE Entry=249420;'
 'custom-phase'="UPDATE gameobject SET PhaseId='7003' WHERE id=249420;"
 'custom-map'='UPDATE gameobject SET map=1220 WHERE id=249420;'
 'custom-extra-loot'='UPDATE gameobject_loot_template SET Entry=249420 WHERE Entry=248834;'
 'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1,Comment) VALUES(4,249420,128866,4,1,'custom');"
 'custom-ai-event'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,comment) VALUES(249420,1,0,64,1,'custom');"
 'custom-spawn-ai'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,comment) VALUES(-140818,1,0,64,1,'custom');"
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before-template',current.* FROM gameobject_template current ORDER BY entry;SELECT '$name-before-gates',current.* FROM conditions current ORDER BY 2,3,4,6,7;`n$migration`nSELECT '$name-after-template',current.* FROM gameobject_template current ORDER BY entry;SELECT '$name-after-gates',current.* FROM conditions current ORDER BY 2,3,4,6,7;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$sql){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Truthguard TEMP real-loot fixture failed'};return $rows}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
[IO.File]::WriteAllLines("$out/fixture.tsv",[string[]]$rows)
function Rows([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
function Signature([string]$label){[string]::Join("`n",[string[]]@(Rows $label))}
foreach($pair in @(@('first-template','second-template'),@('first-gates','second-gates'),@('other-before','other-after'))+@('gameobject_loot_template','smart_scripts','gameobject'|ForEach-Object {,@("$_-before","$_-after")})){if((Signature $pair[0]) -cne (Signature $pair[1])){throw "Changed unrelated/repeated loot content: $pair"}}
foreach($name in $variants.Keys){foreach($suffix in @('template','gates')){if((Signature "$name-before-$suffix") -cne (Signature "$name-after-$suffix")){throw "Custom definition overwritten: $name $suffix"}}}
$before=@(Rows 'before-template');$after=@(Rows 'first-template')
if($before.Count -ne 2 -or $after.Count -ne 2 -or (Rows 'link') -ne '249420' -or @(Rows 'gate').Count -ne 14){throw 'Expected exact loot link and fourteen native rules'}
$weaponIndex=[Array]::FindIndex([string[]]$before,[Predicate[string]]{param($row) $row.Split("`t")[0] -eq '249420'})
if($weaponIndex -lt 0){throw 'Native weapon template missing'}
$first=$before[$weaponIndex].Split("`t");$second=$after[$weaponIndex].Split("`t")
$schema=Query 'SHOW COLUMNS FROM gameobject_template;'
$columns=@($schema|ForEach-Object {$_.Split("`t")[0]});$linkColumn=[Array]::IndexOf($columns,'Data1')
if($linkColumn -lt 0 -or $first[0] -ne '249420' -or $second[$linkColumn] -ne '249420'){throw 'Native link field not found'}
$first[$linkColumn]='249420'
if(($first -join "`t") -cne ($second -join "`t") -or $before[1-$weaponIndex] -cne $after[1-$weaponIndex]){throw 'More than weapon Data1 changed'}
[IO.File]::WriteAllLines("$out/loot-gates.tsv",[string[]](Rows 'gate'))
& "$PSScriptRoot/Test-TalonclawPickupAdmission.ps1" -TruthguardRowsPath "$out/loot-gates.tsv"
if($LASTEXITCODE){throw 'Native Truthguard loot conditions failed'}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TruthguardPickupCatalog.cpp -ProbeName TruthguardPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if($LASTEXITCODE){throw 'Native Truthguard pickup metadata failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv"
foreach($expected in @("CHILD`t128866`t128867`t15","ITEM`t128866`t49`t14`tTruthguard","ITEM`t128867`t0`t21`tOathseeker","CRITERION`t5`t29844`t92`t50545`t0","EFFECT`t210133`t1`t249`t0`t128866`t0`t1`t0","EFFECT`t210133`t0`t61`t0`t50545`t0`t1`t0","EFFECT`t210133`t2`t90`t0`t105891`t0`t1`t0","CAST`t210133`t1`t0`t0","LOCK`t1691`t1`t2`t5`t0")){
 if($native -notcontains $expected){throw "Native Truthguard metadata differs: $expected"}
}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_25_07_truthguard_real_loot.sql" -Algorithm SHA1).Hash.ToLower()
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_25_07_truthguard_real_loot.sql';SELECT 'link',Data1 FROM gameobject_template WHERE entry=249420;SELECT 'gate',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope ORDER BY ElseGroup,ConditionTypeOrReference;"
 if($live -notcontains "release`t$hash`tRELEASED" -or $live -notcontains "link`t249420"){throw 'Real-loot migration not deployed'}
 $deployed=@($live|Where-Object {$_ -like "gate`t*"}|ForEach-Object {$_.Substring(5)})
 if(($deployed -join "`n") -cne (Signature 'gate')){throw 'Live loot conditions differ from native fixture'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_truthguard_real_loot_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical completed-loot implementation failed'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: guarded real-loot link, fourteen native source4 rules, cached-slot admission, idempotence and sixteen custom-preservation variants; no character/runtime data changes by fixture.'
