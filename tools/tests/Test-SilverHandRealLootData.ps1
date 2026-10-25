param([switch]$VerifyReleaseBinary,[switch]$Baseline)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned loot baseline cannot certify fixed Release'}
if($Baseline){& "$PSScriptRoot/Test-SilverHandPickupAdmission.ps1" -LootBaseline;if(!$?){throw 'Pinned loot baseline failed'};return}
& "$PSScriptRoot/Test-SilverHandPickupAdmission.ps1"
if(!$?){throw 'Native completed-loot lifecycle failed'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/silver-hand-real-loot"
New-Item -ItemType Directory -Force $out|Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_25_10_silver_hand_real_loot.sql" -Raw
$scope='SourceTypeOrReferenceId=4 AND SourceGroup=249824'
$tables=@('gameobject_template','gameobject_loot_template','conditions','smart_scripts','gameobject')
$reset=($tables|ForEach-Object {"DELETE FROM $_;INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$reset+="`nUPDATE gameobject_template SET Data1=0 WHERE entry=249824;DELETE FROM conditions WHERE $scope;"
$sql=@'
CREATE TEMPORARY TABLE saved_gameobject_template LIKE gameobject_template;
INSERT INTO saved_gameobject_template SELECT * FROM gameobject_template WHERE entry IN(249824,248831);
CREATE TEMPORARY TABLE saved_gameobject_loot_template LIKE gameobject_loot_template;
INSERT INTO saved_gameobject_loot_template SELECT * FROM gameobject_loot_template WHERE Entry IN(249824,248831);
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup IN(249824,248831);
CREATE TEMPORARY TABLE saved_smart_scripts LIKE smart_scripts;
INSERT INTO saved_smart_scripts SELECT * FROM smart_scripts WHERE entryorguid IN(249824,-134279) AND source_type=1;
CREATE TEMPORARY TABLE saved_gameobject LIKE gameobject;
INSERT INTO saved_gameobject SELECT * FROM gameobject WHERE id IN(249824,248831);
'@
foreach($table in $tables){$sql+="`nCREATE TEMPORARY TABLE $table LIKE saved_$table;"}
$sql+="$reset`nSELECT 'before-template',current.* FROM gameobject_template current ORDER BY entry;`n$migration`nSELECT 'first-template',current.* FROM gameobject_template current ORDER BY entry;SELECT 'first-gates',current.* FROM conditions current ORDER BY 2,3,4,6,7;`n$migration`nSELECT 'second-template',current.* FROM gameobject_template current ORDER BY entry;SELECT 'second-gates',current.* FROM conditions current ORDER BY 2,3,4,6,7;`n"
foreach($table in @('gameobject_loot_template','smart_scripts','gameobject')){$sql+="SELECT '$table-before',current.* FROM saved_$table current ORDER BY 2,3;SELECT '$table-after',current.* FROM $table current ORDER BY 2,3;`n"}
$sql+="SELECT 'other-before',current.* FROM saved_conditions current WHERE NOT($scope) ORDER BY 2,3,4,6,7;SELECT 'other-after',current.* FROM conditions current WHERE NOT($scope) ORDER BY 2,3,4,6,7;SELECT 'gate',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope ORDER BY ElseGroup,ConditionTypeOrReference;SELECT 'link',Data1 FROM gameobject_template WHERE entry=249824;`n"
$variants=[ordered]@{
 'custom-script'="UPDATE gameobject_template SET ScriptName='custom' WHERE entry=249824;"
 'custom-ai'="UPDATE gameobject_template SET AIName='SmartGameObjectAI' WHERE entry=249824;"
 'custom-link'='UPDATE gameobject_template SET Data1=123 WHERE entry=249824;'
 'custom-personal'='UPDATE gameobject_template SET Data30=1 WHERE entry=249824;'
 'custom-group'='UPDATE gameobject_template SET Data15=1 WHERE entry=249824;'
 'custom-security'='UPDATE gameobject_template SET Data31=1 WHERE entry=249824;'
 'custom-vein'='UPDATE gameobject_template SET Data4=1,Data5=2 WHERE entry=249824;'
 'custom-chance'='UPDATE gameobject_loot_template SET Chance=50 WHERE Entry=249824;'
 'custom-reference'='UPDATE gameobject_loot_template SET Reference=1 WHERE Entry=249824;'
 'custom-count'='UPDATE gameobject_loot_template SET MaxCount=2 WHERE Entry=249824;'
 'custom-phase'="UPDATE gameobject SET PhaseId='7003' WHERE id=249824;"
 'custom-map'='UPDATE gameobject SET map=1220 WHERE id=249824;'
 'custom-extra-loot'='UPDATE gameobject_loot_template SET Entry=249824 WHERE Entry=248831;'
 'custom-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1,Comment) VALUES(4,249824,0,4,1,'custom');"
 'custom-ai-event'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,comment) VALUES(249824,1,0,64,1,'custom');"
 'custom-spawn-ai'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,comment) VALUES(-134279,1,0,64,1,'custom');"
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before-template',current.* FROM gameobject_template current ORDER BY entry;SELECT '$name-before-gates',current.* FROM conditions current ORDER BY 2,3,4,6,7;`n$migration`nSELECT '$name-after-template',current.* FROM gameobject_template current ORDER BY entry;SELECT '$name-after-gates',current.* FROM conditions current ORDER BY 2,3,4,6,7;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$rows=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Silver Hand TEMP fixture failed'};return $rows}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
[IO.File]::WriteAllLines("$out/fixture.tsv",[string[]]$rows)
function Rows([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
function Signature([string]$label){[string]::Join("`n",[string[]]@(Rows $label))}
foreach($pair in @(@('first-template','second-template'),@('first-gates','second-gates'),@('other-before','other-after'))+@('gameobject_loot_template','smart_scripts','gameobject'|ForEach-Object {,@("$_-before","$_-after")})){if((Signature $pair[0]) -cne (Signature $pair[1])){throw "Unrelated/repeated loot content changed: $pair"}}
foreach($name in $variants.Keys){foreach($suffix in @('template','gates')){if((Signature "$name-before-$suffix") -cne (Signature "$name-after-$suffix")){throw "Custom definition overwritten: $name $suffix"}}}
$before=@(Rows 'before-template');$after=@(Rows 'first-template')
if($before.Count -ne 2 -or $after.Count -ne 2 -or (Rows 'link') -ne '249824' -or @(Rows 'gate').Count -ne 14){throw 'Expected native loot link and fourteen source4 rows'}
$first=($before|Where-Object {$_ -like "249824`t*"}).Split("`t");$second=($after|Where-Object {$_ -like "249824`t*"}).Split("`t")
$columns=@((Query 'SHOW COLUMNS FROM gameobject_template;')|ForEach-Object {$_.Split("`t")[0]});$linkColumn=[Array]::IndexOf($columns,'Data1')
if($linkColumn -lt 0 -or $second[$linkColumn] -ne '249824'){throw 'Native chest loot field differs'}
$first[$linkColumn]='249824'
if(($first -join "`t") -cne ($second -join "`t") -or $before[0] -cne $after[0]){throw 'More than own Data1 changed'}
[IO.File]::WriteAllLines("$out/loot-gates.tsv",[string[]](Rows 'gate'))
& "$PSScriptRoot/Test-TalonclawPickupAdmission.ps1" -SilverHandRowsPath "$out/loot-gates.tsv"
if(!$? -or $LASTEXITCODE){throw 'Native Silver Hand loot conditions failed'}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_25_10_silver_hand_real_loot.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_25_10_silver_hand_real_loot.sql';SELECT 'link',Data1 FROM gameobject_template WHERE entry=249824;SELECT 'gate',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope ORDER BY ElseGroup,ConditionTypeOrReference;"
 if($live -notcontains "release`t$hash`tRELEASED" -or $live -notcontains "link`t249824"){throw 'Real-loot migration not deployed'}
 $deployed=@($live|Where-Object {$_ -like "gate`t*"}|ForEach-Object {$_.Substring(5)})
 if(($deployed -join "`n") -cne (Signature 'gate')){throw 'Live loot rules differ from tested rules'}
 & "$PSScriptRoot/Test-SilverHandPickupAdmission.ps1" -VerifyReleaseBinary
 if(!$?){throw 'Canonical completed-loot implementation failed'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_real_loot_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical native Silver Hand loot/return consumers failed'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: genuine Silver Hand link/fourteen native rules/cached admission and sixteen custom-preservation variants; native storage/retry/claim separate from unverified client rendering and full return.'
