param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-return-caster"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_23_22_marksmanship_return_caster.sql" -Raw
$sql=@"
CREATE TEMPORARY TABLE spell_script_names (spell_id INT,ScriptName VARCHAR(128));
INSERT INTO spell_script_names VALUES(205040,'custom_return'),(123,'other_spell');
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry=100397;
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
INSERT INTO creature_template SELECT * FROM saved_actor;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=100397 AND source_type=0;
UPDATE saved_smart SET action_type=85 WHERE id=56 AND event_type=61 AND action_type=11 AND action_param1=205040 AND action_param2=0 AND target_type=7;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100397;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
$migration
$migration
SELECT 'binding',COUNT(*) FROM spell_script_names WHERE spell_id=205040 AND ScriptName='spell_marksmanship_return';
SELECT 'custom-binding',COUNT(*) FROM spell_script_names WHERE ScriptName IN('custom_return','other_spell');
SELECT 'row',action_type,action_param1,action_param2,target_type FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=56;
SELECT 'unchanged',COUNT(*) FROM smart_scripts current JOIN saved_smart old ON current.entryorguid=old.entryorguid AND current.source_type=old.source_type AND current.id=old.id AND current.event_type=old.event_type AND current.link=old.link WHERE current.id<>56 AND current.action_type=old.action_type AND current.action_param1=old.action_param1 AND current.action_param2=old.action_param2 AND current.target_type=old.target_type AND current.event_flags=old.event_flags;
SELECT 'saved-unchanged',COUNT(*) FROM saved_smart WHERE id<>56;
SELECT 'link-flags',COUNT(*) FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=56 AND event_type=61 AND event_flags=1 AND target_type=7;
UPDATE smart_scripts SET action_type=85 WHERE id=56;
UPDATE creature_template SET ScriptName='custom_actor' WHERE entry=100397;
$migration
SELECT 'custom-script',COUNT(*) FROM smart_scripts WHERE id=56 AND action_type=85;
UPDATE creature_template SET ScriptName='' WHERE entry=100397;
UPDATE smart_scripts SET action_param2=2 WHERE id=56;
$migration
SELECT 'custom-flags',COUNT(*) FROM smart_scripts WHERE id=56 AND action_type=85 AND action_param2=2;
UPDATE smart_scripts SET action_param2=0,target_type=1 WHERE id=56;
$migration
SELECT 'custom-target',COUNT(*) FROM smart_scripts WHERE id=56 AND action_type=85 AND target_type=1;
UPDATE smart_scripts SET target_type=7,action_param1=123 WHERE id=56;
$migration
SELECT 'custom-spell',COUNT(*) FROM smart_scripts WHERE id=56 AND action_type=85 AND action_param1=123;
UPDATE smart_scripts SET action_param1=205040 WHERE id=56;
UPDATE smart_scripts SET event_flags=1 WHERE id=59;
$migration
SELECT 'one-shot-choice',COUNT(*) FROM smart_scripts WHERE id=56 AND action_type=85;
UPDATE smart_scripts SET event_flags=0 WHERE id=59;
DELETE FROM conditions WHERE SourceGroup=60 AND ElseGroup=1 AND ConditionTypeOrReference=44;
$migration
SELECT 'missing-admission',COUNT(*) FROM smart_scripts WHERE id=56 AND action_type=85;
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Return caster temporary SQL fixture failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("row`t11`t205040`t0`t7","binding`t1","custom-binding`t2","link-flags`t1","custom-script`t1","custom-flags`t1","custom-target`t1","custom-spell`t1","one-shot-choice`t1","missing-admission`t1")){if($rows -notcontains $expected){throw "Return caster mismatch: $expected"}}
if((($rows | Where-Object {$_ -like "unchanged`t*"}) -replace '^unchanged','') -ne (($rows | Where-Object {$_ -like "saved-unchanged`t*"}) -replace '^saved-unchanged','')){throw 'Unrelated actor events changed'}
[IO.File]::WriteAllLines("$out/action.tsv",[string[]]@($rows | Where-Object {$_ -like "row`t*"} | ForEach-Object {$_.Substring(4)}))
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
[IO.File]::WriteAllText("$out/CastAction.inc",(Section $source '        case SMART_ACTION_CAST:' '        case SMART_ACTION_CAST_CUSTOM:'))
[IO.File]::WriteAllText("$out/InvokerCast.inc",(Section $source '        case SMART_ACTION_INVOKER_CAST:' '        case SMART_ACTION_ADD_AURA:'))
[IO.File]::WriteAllText("$out/InvokerTarget.inc",(Section $source '        case SMART_TARGET_NONE:' '        case SMART_TARGET_ACTION_INVOKER_VEHICLE:'))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
[IO.File]::WriteAllText("$out/CastFlags.inc",(Section $source 'enum SmartCastFlags' '// one line in DB'))
$source=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
[IO.File]::WriteAllText("$out/CreditEffect.inc",(Section $source 'void Spell::EffectKillCredit(SpellEffIndex' 'void Spell::EffectQuestFail('))
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
[IO.File]::WriteAllText("$out/GroupCredit.inc",(Section $source 'void Player::RewardPlayerAndGroupAtEvent(' 'bool Player::IsAtGroupRewardDistance('))
$first=$source.IndexOf('            SetDelayedTeleportFlag(IsCanDelayTeleport());',$source.IndexOf('// far teleport to another map'))
$last=$source.IndexOf('            EndMovieMovement();',$first)
if($first -lt 0 -or $last -le $first){throw 'Native far-teleport deferral missing'}
[IO.File]::WriteAllText("$out/TeleportDelay.inc",$source.Substring($first,$last-$first))
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.h" -Raw
[IO.File]::WriteAllText("$out/DelayScope.inc",(Section $source '        class ScopedTeleportDelay' '        bool TeleportToBGEntryPoint();'))
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
$returnEnd=if($source.Contains('namespace MarksmanshipClaim' + "`r`n" + '{') -or $source.Contains('namespace MarksmanshipClaim' + "`n" + '{')){'namespace MarksmanshipClaim'}else{'void AddSC_artifact_acquisition()'}
[IO.File]::WriteAllText("$out/ReturnState.inc",(Section $source 'namespace MarksmanshipReturn' $returnEnd))
[IO.File]::WriteAllText("$out/Active.inc",(Section $source 'bool Active(Player*' 'bool Use(Player*'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/MarksmanshipReturnCasterTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/action.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native return caster regression failed'}
& "$PSScriptRoot/Test-SurvivalArtifactCatalog.ps1"
$native=Get-Content "$repo/build-extractors/runtime-test/artifact-acquisition/survival-native.tsv"
foreach($expected in @("RETURN_CAST`t6`t5000`t3`t20`t20","RETURN_VISUAL`t0`t38616","EFFECT`t205040`t0`t252`t0`t1317`t0`t0","EFFECT`t205040`t1`t90`t0`t102548`t0`t0")){if($native -notcontains $expected){throw "Return native catalog mismatch: $expected"}}
& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$repo/tools/research/audit_marksmanship_return_visual.py" --output "$out/visual.json"
if($LASTEXITCODE){throw 'Native return visual trace failed'}
if($VerifyReleaseBinary){$previousPython=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_marksmanship_return_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical return/SmartAI release chain failed'}}finally{$env:PYTHONPATH=$previousPython}}
'PASS: repeated actual-schema return caster migration preserves original travel/effects/targets/admission and unknown custom definitions'
