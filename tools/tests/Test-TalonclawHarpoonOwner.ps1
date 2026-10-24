param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/talonclaw-harpoon-owner"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_27_talonclaw_harpoon_owner.sql" -Raw
$reset=@'
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart;
UPDATE smart_scripts SET target_type=21,target_param1=100 WHERE id=5;
DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_actor;
DELETE FROM conditions;
DELETE FROM spell_script_names;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=99431;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=99431 AND source_type=0;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
CREATE TEMPORARY TABLE saved_bindings LIKE spell_script_names;
CREATE TEMPORARY TABLE spell_script_names LIKE saved_bindings;
$reset
SELECT 'BEFORE',smart_scripts.* FROM smart_scripts ORDER BY id,link;
$migration
SELECT 'FIRST',smart_scripts.* FROM smart_scripts ORDER BY id,link;
$migration
SELECT 'SECOND',smart_scripts.* FROM smart_scripts ORDER BY id,link;
SELECT 'event',id,link,event_type,action_type,target_type,target_param1,event_flags,action_param1,action_param2,action_param3,action_param4 FROM smart_scripts WHERE id=5;
"@
$variants=[ordered]@{
 'custom-ai'="UPDATE creature_template SET AIName='';"
 'custom-script'="UPDATE creature_template SET ScriptName='custom_harpoon';"
 'custom-vehicle'='UPDATE creature_template SET VehicleId=1;'
 'custom-target'='UPDATE smart_scripts SET target_type=7 WHERE id=5;'
 'custom-distance'='UPDATE smart_scripts SET target_param1=50 WHERE id=5;'
 'custom-asset'='UPDATE smart_scripts SET action_param2=1 WHERE id=5;'
 'custom-repeat'='UPDATE smart_scripts SET event_flags=0 WHERE id=5;'
 'custom-link'='UPDATE smart_scripts SET link=0 WHERE id=5;'
 'custom-timer'='UPDATE smart_scripts SET event_param1=1000 WHERE id=3 AND link=4;'
 'custom-teleport'='UPDATE smart_scripts SET target_type=7 WHERE id=4;'
 'missing-root'='DELETE FROM smart_scripts WHERE id=0;'
 'custom-condition'='INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ConditionTypeOrReference,ConditionValue1) VALUES(22,6,99431,0,9,1);'
 'custom-cast'="INSERT INTO spell_script_names VALUES(195475,'custom_harpoon');"
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',smart_scripts.* FROM smart_scripts ORDER BY id,link;`n$migration`nSELECT '$name-after',smart_scripts.* FROM smart_scripts ORDER BY id,link;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Harpoon temporary SQL failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
function Snapshot([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
$first=Snapshot 'FIRST';$second=Snapshot 'SECOND'
if($first.Count -ne 10 -or (Compare-Object $first $second)){throw 'Harpoon owner migration not idempotent'}
$columns=@(Query 'SHOW COLUMNS FROM smart_scripts;'|ForEach-Object {$_.Split("`t")[0]})
$expected=@(Snapshot 'BEFORE'|ForEach-Object {$fields=$_.Split("`t");if($fields[$columns.IndexOf('id')] -eq '5'){$fields[$columns.IndexOf('target_type')]='23';$fields[$columns.IndexOf('target_param1')]='0'};$fields -join "`t"})
if(Compare-Object $first $expected){throw 'Harpoon migration changed more than owner target/distance'}
foreach($name in $variants.Keys){if(Compare-Object (Snapshot "$name-before") (Snapshot "$name-after")){throw "Custom harpoon overwritten: $name"}}
$event=@($rows|Where-Object {$_ -like "event`t*"}|ForEach-Object {$_.Substring(6)})
if($event.Count -ne 1){throw 'Harpoon credit row not unique'}
if($Baseline){$fields=$event[0].Split("`t");$fields[4]='21';$fields[5]='100';$event=@($fields -join "`t")}
[IO.File]::WriteAllLines("$out/event.tsv",[string[]]$event)
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first) -replace '\be\b','nativeEvent' -replace '\bl\b','targets'}
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
[IO.File]::WriteAllText("$out/Targets.inc",(Section $source '        case SMART_TARGET_CLOSEST_PLAYER:' '        case SMART_TARGET_THREAT_LIST:'))
[IO.File]::WriteAllText("$out/CreditPlayer.inc",(Section $source 'Player* GetScriptCreditPlayer(' 'Unit* GetScriptCreditCaster('))
[IO.File]::WriteAllText("$out/Credit.inc",(Section $source '        case SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA:' '        case SMART_ACTION_SUMMON_CONVERSATION:'))
[IO.File]::WriteAllText("$out/RepeatGuard.inc",(Section $source '    if ((e.event.event_phase_mask && !IsInPhase(' '    ConditionList conds ='))
[IO.File]::WriteAllText("$out/FindLink.inc",(Section $source 'SmartScriptHolder SmartScript::FindLinkedEvent(' 'void SmartScript::OnUpdate('))
$follow=Section $source '    if (e.link && e.link != e.event_id)' 'void SmartScript::InstallTemplate('
[IO.File]::WriteAllText("$out/FollowLink.inc",$follow.Substring(0,$follow.LastIndexOf('}')))
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
[IO.File]::WriteAllText("$out/Owner.inc",(Section $source 'Unit* Unit::GetAnyOwner() const' 'Unit* Unit::GetCharmer() const'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/TalonclawHarpoonOwnerTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/event.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native harpoon owner regression failed'}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TalonclawHarpoonCatalog.cpp -ProbeName TalonclawHarpoonCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv"
foreach($expected in @("SUMMON`t195475`tThrow Harpoon`t0`t46`t0`t64","CRITERION`t29205`t92`t47608","BOARDING_SUMMON`t61`t1`t2`t2","HARPOON_SUMMON`t64`t0`t0`t0")){if($native -notcontains $expected){throw "Harpoon native record changed: $expected"}}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_27_talonclaw_harpoon_owner.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_27_talonclaw_harpoon_owner.sql'; SELECT 'owner',target_type,target_param1 FROM smart_scripts WHERE entryorguid=99431 AND source_type=0 AND id=5 AND link=6;"
 foreach($expected in @("release`t$hash`tRELEASED","owner`t23`t0")){if($live -notcontains $expected){throw "Harpoon deployment mismatch: $expected"}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_talonclaw_harpoon_owner_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Harpoon canonical native paths missing'}}finally{$env:PYTHONPATH=$previous}
}
"PASS: guarded owner27 changes exactly two target fields, idempotent and$($variants.Count) custom variants retained; native ownership/credit/LINK regression and26972 data checked, client arrival pending"
