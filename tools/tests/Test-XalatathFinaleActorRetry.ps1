param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/xalatath-finale-actor-retry"
$weapon="$repo/build-extractors/runtime-test/xalatath-weapon-summon-retry"
$retry="$repo/build-extractors/runtime-test/marksmanship-bow-summon-retry"
$reveal="$repo/build-extractors/runtime-test/marksmanship-bow-reveal"
$search="$repo/build-extractors/runtime-test/marksmanship-search-path"
$common="$repo/build-extractors/runtime-test/marksmanship-herald-challenge"
New-Item -ItemType Directory -Force $out | Out-Null
& "$PSScriptRoot/Test-XalatathWeaponSummonRetry.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary *> "$out/native-prerequisite.log"
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_44_xalatath_finale_actor_retry.sql" -Raw
$normalization=Get-Content "$PSScriptRoot/XalatathActorRetryBaseline.sql" -Raw
$reset=@'
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart;
DELETE FROM conditions; INSERT INTO conditions SELECT * FROM saved_conditions;
DELETE FROM creature_template; INSERT INTO creature_template SELECT * FROM saved_actor;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=101148;
$normalization
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=101148 AND SourceGroup NOT IN(33,34);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
CREATE TEMPORARY TABLE saved_actor LIKE creature_template;
INSERT INTO saved_actor SELECT * FROM creature_template WHERE entry IN(101148,102247);
CREATE TEMPORARY TABLE creature_template LIKE saved_actor;
$reset
SELECT 'unchanged-before',smart_scripts.* FROM smart_scripts WHERE NOT(source_type=0 AND id IN(26,29)) ORDER BY source_type,id;
SELECT 'actors-before',creature_template.* FROM creature_template ORDER BY entry;
$migration
SELECT 'first',smart_scripts.* FROM smart_scripts ORDER BY source_type,id;
SELECT 'first-gates',conditions.* FROM conditions ORDER BY SourceGroup,SourceId,ElseGroup,ConditionTypeOrReference;
$migration
SELECT 'second',smart_scripts.* FROM smart_scripts ORDER BY source_type,id;
SELECT 'second-gates',conditions.* FROM conditions ORDER BY SourceGroup,SourceId,ElseGroup,ConditionTypeOrReference;
SELECT 'unchanged-after',smart_scripts.* FROM smart_scripts WHERE NOT(source_type=0 AND id IN(26,29,32,33)) ORDER BY source_type,id;
SELECT 'actors-after',creature_template.* FROM creature_template ORDER BY entry;
SELECT 'count',COUNT(*) FROM smart_scripts;
SELECT 'gate-count',COUNT(*) FROM conditions WHERE SourceGroup IN(33,34) AND SourceId=0;
SELECT 'row',entryorguid,source_type,id,link,event_type,event_phase_mask,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,action_param3,target_type,target_x,target_y,target_z,target_o FROM smart_scripts WHERE source_type=0 AND id IN(26,27,28,29,32,33) ORDER BY id;
SELECT 'gate',SourceGroup,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceGroup IN(33,34) AND SourceId=0 ORDER BY SourceGroup,ConditionTypeOrReference;
"@
$variants=[ordered]@{
 'custom-owner'="UPDATE creature_template SET ScriptName='custom' WHERE entry=101148;"
 'custom-child'="UPDATE creature_template SET ScriptName='custom' WHERE entry=102247;"
 'custom-health'='UPDATE smart_scripts SET event_param2=2 WHERE source_type=0 AND id=25;'
 'custom-position'='UPDATE smart_scripts SET target_z=1 WHERE source_type=0 AND id=26;'
 'custom-duration'='UPDATE smart_scripts SET action_param3=1 WHERE source_type=0 AND id=26;'
 'custom-type'='UPDATE smart_scripts SET action_param2=1 WHERE source_type=0 AND id=26;'
 'custom-target'='UPDATE smart_scripts SET target_type=7 WHERE source_type=0 AND id=26;'
 'custom-link'='UPDATE smart_scripts SET link=0 WHERE source_type=0 AND id=26;'
 'custom-disarm'='UPDATE smart_scripts SET action_param1=1 WHERE source_type=0 AND id=27;'
 'custom-conversation'='UPDATE smart_scripts SET action_param1=1 WHERE source_type=0 AND id=28;'
 'custom-hit'='UPDATE smart_scripts SET event_param1=1 WHERE source_type=0 AND id=29;'
 'custom-phase'='UPDATE smart_scripts SET event_phase_mask=2 WHERE source_type=0 AND id=24;'
 'custom-phase-action'='UPDATE smart_scripts SET action_type=22 WHERE source_type=0 AND id=24;'
 'occupied-id'="INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,comment) VALUES(101148,0,32,60,1,'Custom');"
 'custom-gate'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ConditionTypeOrReference,ConditionValue1,Comment) VALUES(22,33,101148,0,22,1539,'Custom');"
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',smart_scripts.* FROM smart_scripts ORDER BY source_type,id;`nSELECT '$name-gates-before',conditions.* FROM conditions ORDER BY SourceGroup,SourceId,ElseGroup,ConditionTypeOrReference;`n$migration`nSELECT '$name-after',smart_scripts.* FROM smart_scripts ORDER BY source_type,id;`nSELECT '$name-gates-after',conditions.* FROM conditions ORDER BY SourceGroup,SourceId,ElseGroup,ConditionTypeOrReference;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Actor retry TEMP query failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
[IO.File]::WriteAllLines("$out/fixture-diagnostics.tsv",[string[]]$rows)
function Snapshot([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
function Signature([string]$label){[string]::Join("`n",[string[]]@(Snapshot $label))}
foreach($expected in @("count`t36","gate-count`t6")){if($rows -notcontains $expected){throw "Actor retry mismatch $expected"}}
foreach($pair in @(@('first','second'),@('first-gates','second-gates'),@('unchanged-before','unchanged-after'),@('actors-before','actors-after'))){if((Signature $pair[0]) -cne (Signature $pair[1])){throw "Snapshot mismatch $pair"}}
foreach($name in $variants.Keys){foreach($suffix in @('','-gates')){if((Signature "$name$suffix-before") -cne (Signature "$name$suffix-after")){throw "Custom content overwritten: $name"}}}
foreach($kind in @('row','gate')){[IO.File]::WriteAllLines("$out/$kind.tsv",[string[]](Snapshot $kind))}
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing section $start"};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$action=Section $source '        case SMART_ACTION_SUMMON_CREATURE:' '        case SMART_ACTION_SUMMON_GO:'
$first=$action.LastIndexOf('            if (e.GetTargetType() == SMART_TARGET_POSITION)');$last=$action.LastIndexOf('            break;')
if($first -lt 0 -or $last -le $first){throw 'Native fixed summon branch missing'}
[IO.File]::WriteAllText("$out/FixedCreatureSummon.inc",$action.Substring($first,$last-$first))
[IO.File]::WriteAllText("$out/SummonedEvent.inc",(Section $source '        case SMART_EVENT_SUMMONED_UNIT:' '        case SMART_EVENT_RECEIVE_HEAL:'))
[IO.File]::WriteAllText("$out/SpellHitEvent.inc",(Section $source '        case SMART_EVENT_SPELLHIT_TARGET:' '        case SMART_EVENT_OOC_LOS:'))
[IO.File]::WriteAllText("$out/NativeReset.inc",(Section $source 'void SmartScript::OnReset()' 'void SmartScript::ResetBaseObject()'))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
[IO.File]::WriteAllText("$out/NativeJustSummoned.inc",(Section $source 'void SmartAI::JustSummoned(' 'void SmartAI::AttackStart('))
[IO.File]::WriteAllText("$out/NativeSpellHit.inc",(Section $source 'void SmartAI::SpellHit(' 'void SmartAI::SpellHitTarget('))
$source=Get-Content "$repo/src/server/game/Entities/Creature/TemporarySummon.cpp" -Raw
[IO.File]::WriteAllText("$out/NativeInitSummon.inc",(Section $source 'void TempSummon::InitSummon()' 'bool TempSummon::InitBaseStat('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" /I`"$PSScriptRoot`" /I`"$weapon`" /I`"$retry`" /I`"$reveal`" /I`"$search`" /I`"$common`" `"$PSScriptRoot/XalatathFinaleActorRetryTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/row.tsv`" `"$out/gate.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native actor retry regression failed'}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_44_xalatath_finale_actor_retry.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_44_xalatath_finale_actor_retry.sql'; SELECT 'row',entryorguid,source_type,id,link,event_type,event_phase_mask,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,action_param3,target_type,target_x,target_y,target_z,target_o FROM smart_scripts WHERE entryorguid=101148 AND source_type=0 AND id IN(26,27,28,29,32,33) ORDER BY id; SELECT 'gate',SourceGroup,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=101148 AND SourceGroup IN(33,34) AND SourceId=0 ORDER BY SourceGroup,ConditionTypeOrReference;"
 if($live -notcontains "release`t$hash`tRELEASED"){throw 'Live actor retry hash mismatch'}
 foreach($kind in @('row','gate')){$current=@($live|Where-Object {$_ -like "$kind`t*"}|ForEach-Object {$_.Substring($kind.Length+1)});if(Compare-Object (Snapshot $kind) $current){throw 'Live actor retry differs from tested graph'}}
 $previousPython=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previousPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_xalatath_finale_actor_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical actor consumer failed'}}finally{$env:PYTHONPATH=$previousPython}
}
'PASS: Farthing retries null native soul-actor creation only in1539/991/7; actual summoned callback stops creation, actual201899 starts existing dialogue/list. No item, credit or synthetic hit is granted; map insertion and full client lifecycle remain separate.'
