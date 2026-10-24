param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/xalatath-opening"
New-Item -ItemType Directory -Force $out | Out-Null
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
$acquisition=Section $source 'namespace ArtifactAcquisition' 'namespace DarkRidersCompletion'
$acquisition=$acquisition.Substring($acquisition.IndexOf('{')+1)
[IO.File]::WriteAllText("$out/Acquisition.inc",$acquisition.Substring(0,$acquisition.LastIndexOf('}')))
$binding=Section $source 'namespace XalatathPickup' 'void AddSC_artifact_acquisition()'
if($Baseline){$binding='class go_xalatath_pickup : public GameObjectScript { public: go_xalatath_pickup() : GameObjectScript("baseline_no_binding") {} };'}
[IO.File]::WriteAllText("$out/Binding.inc",$binding)
[IO.File]::WriteAllText("$out/BindingChoice.inc",'using PickupBinding = go_xalatath_pickup;')
$hostSource=Get-Content "$PSScriptRoot/MarksmanshipReportUseTest.cpp" -Raw
[IO.File]::WriteAllText("$out/Host.inc",$hostSource.Substring(0,$hostSource.IndexOf('int main()')))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
[IO.File]::WriteAllText("$out/SmartLifecycle.inc",(Section $source 'SmartGameObjectAI::SmartGameObjectAI(' '// Called when a player selects a gossip item'))
[IO.File]::WriteAllText("$out/StateChanged.inc",(Section $source 'void SmartGameObjectAI::OnStateChanged(' 'void SmartGameObjectAI::EventInform('))
$source=Get-Content "$repo/src/server/game/AI/CreatureAISelector.cpp" -Raw
$first=$source.IndexOf('    GameObjectAI* SelectGameObjectAI(')
if($first -lt 0){throw 'Native GO selector missing'}
[IO.File]::WriteAllText("$out/SelectAI.inc",$source.Substring($first,$source.LastIndexOf('}')-$first))
$source=Get-Content "$repo/src/server/game/Handlers/SpellHandler.cpp" -Raw
[IO.File]::WriteAllText("$out/ReportUse.inc",(Section $source 'void WorldSession::HandleGameobjectReportUse(' 'void WorldSession::HandleCastSpellOpcode('))
$source=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
$send=Section $source 'void Spell::SendLoot(' 'switch (gameObjTarget->GetGoType())'
if(!$send.Contains('sScriptMgr->OnGossipHello') -or !$send.Contains('gameObjTarget->AI()->GossipHello')){throw 'Native open-lock admission changed'}
[IO.File]::WriteAllText("$out/SendLoot.inc",$send+"}`n ++lootRequests;`n}`n")
$use=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
$use=Section $use 'void GameObject::Use(Unit* user)' '    // If cooldown data present in template'
if($use.IndexOf('sScriptMgr->OnGossipHello(') -lt 0 -or $use.IndexOf('AI()->GossipUse(') -le $use.IndexOf('sScriptMgr->OnGossipHello(')){throw 'Ordinary-use pre-mutation gate order changed'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/XalatathOpeningTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Xalatath opening failed'}
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_39_xalatath_opening_admission.sql" -Raw
$reset=@'
DELETE FROM gameobject_template; INSERT INTO gameobject_template SELECT * FROM saved_object;
UPDATE gameobject_template SET ScriptName='' WHERE entry=247690;
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart;
DELETE FROM conditions; INSERT INTO conditions SELECT * FROM saved_conditions;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_object LIKE gameobject_template;
INSERT INTO saved_object SELECT * FROM gameobject_template WHERE entry=247690;
CREATE TEMPORARY TABLE gameobject_template LIKE saved_object;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=247690 AND source_type=1;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceEntry=247690 OR (SourceTypeOrReferenceId=4 AND SourceGroup=247690);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
$reset
SELECT 'object-before',gameobject_template.* FROM gameobject_template;
SELECT 'smart-before',smart_scripts.* FROM smart_scripts ORDER BY id;
SELECT 'cond-before',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;
$migration
SELECT 'first',gameobject_template.* FROM gameobject_template;
$migration
SELECT 'second',gameobject_template.* FROM gameobject_template;
SELECT 'smart-after',smart_scripts.* FROM smart_scripts ORDER BY id;
SELECT 'cond-after',conditions.* FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference;
"@
$variants=[ordered]@{
 'custom-ai'="UPDATE gameobject_template SET AIName='';"
 'custom-script'="UPDATE gameobject_template SET ScriptName='custom';"
 'custom-type'='UPDATE gameobject_template SET type=10;'
 'custom-lock'='UPDATE gameobject_template SET Data0=1;'
 'custom-loot'='UPDATE gameobject_template SET Data1=0;'
 'custom-open-text'='UPDATE gameobject_template SET Data14=1;'
 'custom-scene'='UPDATE smart_scripts SET action_param1=1 WHERE id=0;'
 'custom-link'='UPDATE smart_scripts SET link=0 WHERE id=1;'
 'custom-target'='UPDATE smart_scripts SET target_type=21 WHERE id=2;'
 'missing-action'='DELETE FROM smart_scripts WHERE id=3;'
 'custom-condition'='UPDATE conditions SET ConditionValue1=4 WHERE SourceTypeOrReferenceId=22 AND ConditionTypeOrReference=15;'
 'missing-condition'='DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND ElseGroup=0 AND ConditionTypeOrReference=36;'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',gameobject_template.* FROM gameobject_template;`n$migration`nSELECT '$name-after',gameobject_template.* FROM gameobject_template;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Opening TEMP query failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
function Snapshot([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
if(Compare-Object (Snapshot 'first') (Snapshot 'second')){throw 'Binding not idempotent'}
$columns=@(Query 'SHOW COLUMNS FROM gameobject_template;'|ForEach-Object {$_.Split("`t")[0]})
$expected=@(Snapshot 'object-before'|ForEach-Object {$fields=$_.Split("`t");$fields[$columns.IndexOf('ScriptName')]='go_xalatath_pickup';$fields -join "`t"})
if(Compare-Object $expected (Snapshot 'first')){throw 'Changed fields beyond script binding'}
foreach($kind in @('smart','cond')){if(Compare-Object (Snapshot "$kind-before") (Snapshot "$kind-after")){throw "Changed native $kind"}}
foreach($name in $variants.Keys){if(Compare-Object (Snapshot "$name-before") (Snapshot "$name-after")){throw "Custom variant changed: $name"}}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_39_xalatath_opening_admission.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_39_xalatath_opening_admission.sql'; SELECT 'binding',AIName,ScriptName FROM gameobject_template WHERE entry=247690;"
 foreach($expected in @("release`t$hash`tRELEASED","binding`tSmartGameObjectAI`tgo_xalatath_pickup")){if($live -notcontains $expected){throw "Live opening mismatch: $expected"}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_xalatath_opening_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical opening gate missing'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: Xalatath normal/open-lock/report-use admission, actual-owner native gossip without re-loot,12 custom variants and preserved metadata/conditions/actions. Full loot/client traversal and cast/replay failures remain separate.'
