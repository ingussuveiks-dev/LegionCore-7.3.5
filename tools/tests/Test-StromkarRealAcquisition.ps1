param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot certify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stromkar-real-acquisition"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_45_stromkar_real_acquisition.sql" -Raw
$scope="(SourceTypeOrReferenceId=4 AND SourceGroup=247877) OR (SourceTypeOrReferenceId=22 AND SourceEntry=247877 AND SourceId=1) OR (SourceTypeOrReferenceId=17 AND SourceEntry IN(206436,206448))"
$reset=@"
DELETE FROM gameobject_template; INSERT INTO gameobject_template SELECT * FROM saved_object;
UPDATE gameobject_template SET Data1=0 WHERE entry=247877;
DELETE FROM gameobject_loot_template; INSERT INTO gameobject_loot_template SELECT * FROM saved_loot;
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart;
DELETE FROM conditions; INSERT INTO conditions SELECT * FROM saved_conditions;
DELETE FROM conditions WHERE $scope;
DELETE FROM spell_script_names; INSERT INTO spell_script_names SELECT * FROM saved_bindings;
"@
$sql=@"
CREATE TEMPORARY TABLE saved_object LIKE gameobject_template;
INSERT INTO saved_object SELECT * FROM gameobject_template WHERE entry=247877;
CREATE TEMPORARY TABLE gameobject_template LIKE saved_object;
CREATE TEMPORARY TABLE saved_loot LIKE gameobject_loot_template;
INSERT INTO saved_loot SELECT * FROM gameobject_loot_template WHERE Entry=247877;
CREATE TEMPORARY TABLE gameobject_loot_template LIKE saved_loot;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=247877 AND source_type=1;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceEntry IN(247877,128910,206436,206448) OR (SourceTypeOrReferenceId=4 AND SourceGroup IN(247877,247690));
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
CREATE TEMPORARY TABLE saved_bindings LIKE spell_script_names;
INSERT INTO saved_bindings SELECT * FROM spell_script_names WHERE spell_id IN(206436,206448);
CREATE TEMPORARY TABLE spell_script_names LIKE saved_bindings;
$reset
SELECT 'object-before',gameobject_template.* FROM gameobject_template;
SELECT 'smart-before',smart_scripts.* FROM smart_scripts ORDER BY id;
SELECT 'loot-before',gameobject_loot_template.* FROM gameobject_loot_template;
SELECT 'other-before',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;
$migration
SELECT 'first',gameobject_template.* FROM gameobject_template;
SELECT 'first-gates',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;
$migration
SELECT 'second',gameobject_template.* FROM gameobject_template;
SELECT 'second-gates',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;
SELECT 'smart-after',smart_scripts.* FROM smart_scripts ORDER BY id;
SELECT 'loot-after',gameobject_loot_template.* FROM gameobject_loot_template;
SELECT 'other-after',conditions.* FROM conditions WHERE NOT($scope) ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;
SELECT 'count',COUNT(*) FROM conditions WHERE $scope;
SELECT 'row-loot',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=247877 ORDER BY ElseGroup,ConditionTypeOrReference;
SELECT 'row-root',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=247877 AND SourceId=1 ORDER BY ElseGroup,ConditionTypeOrReference;
SELECT 'row-scene',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=206436 ORDER BY ElseGroup,ConditionTypeOrReference;
SELECT 'row-bar',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=206448 ORDER BY ElseGroup,ConditionTypeOrReference;
"@
$variants=[ordered]@{
 'custom-ai'="UPDATE gameobject_template SET AIName='';"
 'custom-script'="UPDATE gameobject_template SET ScriptName='custom';"
 'custom-type'='UPDATE gameobject_template SET type=10;'
 'custom-lock'='UPDATE gameobject_template SET Data0=1;'
 'custom-open-text'='UPDATE gameobject_template SET Data14=1;'
 'existing-binding'='UPDATE gameobject_template SET Data1=247877;'
 'custom-binding'='UPDATE gameobject_template SET Data1=1;'
 'missing-loot'='DELETE FROM gameobject_loot_template;'
 'custom-item'='UPDATE gameobject_loot_template SET Item=1;'
 'custom-reference'='UPDATE gameobject_loot_template SET Reference=1;'
 'custom-chance'='UPDATE gameobject_loot_template SET Chance=50;'
 'custom-count'='UPDATE gameobject_loot_template SET MaxCount=2;'
 'custom-root'='UPDATE smart_scripts SET action_param2=1 WHERE id=0;'
 'custom-link'='UPDATE smart_scripts SET link=0 WHERE id=1;'
 'custom-scene'='UPDATE smart_scripts SET action_param1=1 WHERE id=1;'
 'custom-bar'='UPDATE smart_scripts SET action_param1=1 WHERE id=2;'
 'existing-loot-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(4,247877,128910,4,1);"
 'existing-root-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ConditionTypeOrReference,ConditionValue1) VALUES(22,1,247877,1,4,1);"
 'existing-cast-condition'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceEntry,ConditionTypeOrReference,ConditionValue1) VALUES(17,206436,4,1);"
 'custom-spell-script'="INSERT INTO spell_script_names(spell_id,ScriptName) VALUES(206448,'custom');"
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',gameobject_template.* FROM gameobject_template;`nSELECT '$name-gates-before',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;`n$migration`nSELECT '$name-after',gameobject_template.* FROM gameobject_template;`nSELECT '$name-gates-after',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Stromkar TEMP query failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
[IO.File]::WriteAllLines("$out/fixture-diagnostics.tsv",[string[]]$rows)
function Snapshot([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
function Signature([string]$label){[string]::Join("`n",[string[]]@(Snapshot $label))}
if($rows -notcontains "count`t52"){throw 'Expected52 scoped admission rows'}
foreach($pair in @(@('first','second'),@('first-gates','second-gates'),@('smart-before','smart-after'),@('loot-before','loot-after'),@('other-before','other-after'),@('row-root','row-scene'),@('row-root','row-bar'))){if((Signature $pair[0]) -cne (Signature $pair[1])){throw "Snapshot mismatch $pair"}}
$columns=@(Query 'SHOW COLUMNS FROM gameobject_template;'|ForEach-Object {$_.Split("`t")[0]})
$expected=@(Snapshot 'object-before'|ForEach-Object {$fields=$_.Split("`t");$fields[$columns.IndexOf('Data1')]='247877';$fields -join "`t"})
if(Compare-Object $expected (Snapshot 'first')){throw 'Object fields beyond Data1 changed'}
foreach($name in $variants.Keys){foreach($suffix in @('','-gates')){if((Signature "$name$suffix-before") -cne (Signature "$name$suffix-after")){throw "Custom content changed: $name"}}}
foreach($kind in @('loot','root','scene','bar')){[IO.File]::WriteAllLines("$out/$kind.tsv",[string[]](Snapshot "row-$kind"))}
& "$PSScriptRoot/Test-TalonclawPickupAdmission.ps1" -StromkarRowsPath "$out/loot.tsv" -StromkarLootId $(if($Baseline){0}else{247877}) *> "$out/loot-native.log"
if(!$?){throw 'Native Stromkar loot consumer failed; see loot-native.log'}
& "$PSScriptRoot/Test-TalonclawPickupAdmission.ps1" -StromkarRowsPath "$out/root.tsv" -StromkarRoot *> "$out/root-native.log"
if(!$?){throw 'Native Stromkar root/predicate matrix failed'}
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
[IO.File]::WriteAllText("$out/CastConditions.inc",(Section $source '    ConditionSourceInfo condInfo = ConditionSourceInfo(m_caster);' "    // Don't check explicit target for passive spells"))
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/NotGroupedConditions.inc",(Section $source 'ConditionList ConditionMgr::GetConditionsForNotGroupedEntry(' 'ConditionList ConditionMgr::GetConditionsForSpellClickEvent('))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$first=$source.IndexOf('tempLastInvoker->AddDelayedEvent(200,');$last=$source.IndexOf('});',$first)
if($first -lt 0 -or $last -lt $first){throw 'Native deferred cast missing'}
[IO.File]::WriteAllText("$out/DeferredInvoker.inc",$source.Substring($first,$last+3-$first))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($spell in @(206436,206448)){
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /DPICKUP_SPELL_ID=$spell /I`"$out`" `"$PSScriptRoot/TitanstrikePickupCastTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw "Native delayed cast failed: $spell"}
}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile StromkarPickupCatalog.cpp -ProbeName StromkarPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$?){throw 'Native Stromkar catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv" -Raw
foreach($expected in @("ITEM`t128910`t1`t1`t-1`t36","EFFECT`t206436`t0`t6`t430`t1225","EFFECT`t206439`t1`t249`t0`t128910","EFFECT`t206448`t0`t6`t293`t684","CRITERION`t7`t29550`t92`t49101","CRITERION`t8`t29551`t92`t49108","CRITERION`t9`t29552`t92`t49109","CRITERION`t9`t29553`t92`t49110")){if(!$native.Contains($expected)){throw "Native evidence changed: $expected"}}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_45_stromkar_real_acquisition.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_45_stromkar_real_acquisition.sql'; SELECT 'binding',type,Data0,Data1,Data14,AIName,ScriptName FROM gameobject_template WHERE entry=247877; SELECT 'scenario',ScenarioID,Class FROM scenario_data WHERE MapID=1539 AND Class=1; SELECT 'row-loot',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=247877 ORDER BY ElseGroup,ConditionTypeOrReference; SELECT 'row-root',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=247877 AND SourceId=1 ORDER BY ElseGroup,ConditionTypeOrReference; SELECT 'row-scene',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=206436 ORDER BY ElseGroup,ConditionTypeOrReference; SELECT 'row-bar',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=206448 ORDER BY ElseGroup,ConditionTypeOrReference;"
 foreach($expected in @("release`t$hash`tRELEASED","binding`t3`t1691`t247877`t23645`tSmartGameObjectAI`t","scenario`t1037`t1")){if($live -notcontains $expected){throw "Live Stromkar mismatch: $expected"}}
 foreach($kind in @('loot','root','scene','bar')){$current=@($live|Where-Object {$_ -like "row-$kind`t*"}|ForEach-Object {$_.Substring(5+$kind.Length)});if(Compare-Object (Snapshot "row-$kind") $current){throw "Live $kind admission differs"}}
 $previousPython=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";foreach($script in @('verify_xalatath_real_loot_release.py','verify_xalatath_progression_admission_release.py','verify_titanstrike_pickup_cast_release.py')){& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/$script" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw "Canonical shared consumer failed: $script"}}}finally{$env:PYTHONPATH=$previousPython}
}
'PASS: real Stromkar loot binding and native Warrior41105/map1539/scenario1037/exact9 loot, owned/living progression and both delayed casts. Actual equip/scene/full chain and physical opening remain separate.'
