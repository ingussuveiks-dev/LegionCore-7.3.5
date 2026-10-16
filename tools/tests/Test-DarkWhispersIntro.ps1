param([string]$MigrationFile="$PSScriptRoot/../../sql/updates/world/2026_10_16_08_dark_whispers_intro.sql",[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-DarkWhispersBeamHook.ps1"
$out="$repo/build-extractors/runtime-test/dark-whispers-intro"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Extract([string]$path,[string]$start,[string]$end,[string]$name){
 $s=Get-Content "$repo/$path" -Raw;$a=$s.IndexOf($start);$b=$s.IndexOf($end,$a)
 if($a -lt 0 -or $b -lt $a){throw "Missing production section $start"}
 [IO.File]::WriteAllText("$out/$name.inc",$s.Substring($a,$b-$a))
}
Extract 'src/server/scripts/Legion/ClassHalls/dark_whispers_artifact.cpp' 'namespace DarkWhispersIntro' 'class spell_dark_whispers_ride_skull' 'Intro'
Extract 'src/server/game/Entities/Unit/Unit.cpp' 'void Unit::InterruptNonMeleeSpells(' 'Spell* Unit::FindCurrentSpellBySpellId(' 'Interrupt'
Extract 'src/server/game/AI/SmartScripts/SmartAI.cpp' 'void SmartAI::EnterCombat(' 'void SmartAI::JustDied(' 'EnterCombat'
Extract 'src/server/game/AI/SmartScripts/SmartAI.cpp' 'void SmartAI::AttackStart(' 'void SmartAI::SpellHit(' 'AttackStart'
Extract 'src/server/game/Entities/Creature/TemporarySummon.cpp' 'void TempSummon::InitSummon()' 'bool TempSummon::InitBaseStat(' 'InitSummon'
Extract 'src/server/game/AI/SmartScripts/SmartScript.cpp' '        case SMART_TARGET_CLOSEST_PLAYER:' '        case SMART_TARGET_OWNER_OR_SUMMONER:' 'ClosestPlayer'
Extract 'src/server/game/AI/SmartScripts/SmartScript.cpp' '        case SMART_ACTION_ATTACK_START:' '        case SMART_ACTION_SUMMON_CREATURE:' 'AttackAction'
Extract 'src/server/game/AI/SmartScripts/SmartScript.cpp' '        case SMART_EVENT_UPDATE_OOC:' '        case SMART_EVENT_UPDATE_IC:' 'OocEvent'
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/DarkWhispersIntroTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Dark Whispers intro native regression failed'}
$cmd=$cmd.Replace('DarkWhispersIntroTest.cpp','DarkWhispersAggroTest.cpp');[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Dark Whispers native aggro regression failed'}
$migration=Get-Content $MigrationFile -Raw
$snapshot=@'
SELECT 'MOVEMENT',Ground,Flight FROM creature_template_movement WHERE CreatureId=106693;
SELECT 'SAI',entryorguid,id,link,event_type,event_phase_mask,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,action_param3,target_type,target_param1,target_param2 FROM smart_scripts WHERE source_type=0 AND entryorguid IN(106692,106693,106698,106644) ORDER BY entryorguid,id;
SELECT 'TAL',id,event_param1,event_param2,action_type,action_param1,action_param2,target_type,target_param1 FROM smart_scripts WHERE entryorguid=10669200 AND source_type=9 ORDER BY id;
SELECT 'BIND',spell_id,ScriptName FROM spell_script_names WHERE spell_id IN(211703,211704,212497) ORDER BY spell_id,ScriptName;
SELECT 'GATE',ConditionTarget,ConditionValue1,ConditionValue2 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=106644 AND SourceGroup=1;
SELECT 'RECOVER',ConditionTarget,ConditionValue1,ConditionValue2 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=106644 AND SourceGroup=2;
SELECT 'TARGET',SourceGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2 FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=211703;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_sai LIKE smart_scripts;
INSERT INTO saved_sai SELECT * FROM smart_scripts;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_sai;
INSERT INTO smart_scripts SELECT * FROM saved_sai;
CREATE TEMPORARY TABLE saved_move LIKE creature_template_movement;
INSERT INTO saved_move SELECT * FROM creature_template_movement;
CREATE TEMPORARY TABLE creature_template_movement LIKE saved_move;
INSERT INTO creature_template_movement SELECT * FROM saved_move;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
CREATE TEMPORARY TABLE saved_bind LIKE spell_script_names;
INSERT INTO saved_bind SELECT * FROM spell_script_names;
CREATE TEMPORARY TABLE spell_script_names LIKE saved_bind;
INSERT INTO spell_script_names SELECT * FROM saved_bind;
$migration
SELECT 'FIRST';
$snapshot
$migration
SELECT 'SECOND';
$snapshot
SELECT 'END';
SELECT 'OTHER',COUNT(*) FROM smart_scripts a JOIN saved_sai b USING(entryorguid,source_type,id) WHERE a.entryorguid=106652 AND a.action_type<>b.action_type;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Intro SQL fixture failed'}}finally{$env:MYSQL_PWD=$previous}
$a=[Array]::IndexOf($rows,'FIRST');$b=[Array]::IndexOf($rows,'SECOND');$d=[Array]::IndexOf($rows,'END')
$first=@($rows[($a+1)..($b-1)]);$second=@($rows[($b+1)..($d-1)])
if(($first -join "`n") -ne ($second -join "`n")){throw 'Intro migration is not idempotent'}
foreach($expected in @("MOVEMENT`t0`t1","GATE`t1`t1097`t1","RECOVER`t1`t1097`t0","TARGET`t1`t31`t0`t3`t106693","BIND`t211703`tspell_dark_whispers_ride_skull","BIND`t211704`tspell_dark_whispers_despawn_skull","BIND`t212497`taura_dark_whispers_ritual_beam")){if($first -notcontains $expected){throw "Missing intro data $expected"}}
$sai=@{};$tal=@{}
foreach($row in $first){$v=$row.Split("`t");if($v[0] -eq 'SAI'){$sai["$($v[1]):$($v[2])"]=$v};if($v[0] -eq 'TAL'){$tal[[int]$v[1]]=$v}}
foreach($id in @(1,2,4)){$v=$sai["106692:$id"];if([int]$v[13] -lt @(0,16006,7261,0,6881)[$id]){throw 'Voice truncated by text-over timer'}}
if($sai['106692:3'][11] -ne '22' -or $sai['106692:11'][12] -ne '211703' -or $sai.ContainsKey('106693:1')){throw 'Skull still despawns on a pickup attempt'}
if($sai['106692:12'][4] -ne '27' -or $sai['106692:12'][12] -ne '211683' -or $sai['106692:13'][5] -ne '1' -or $sai['106692:14'][12] -ne '2'){throw 'Native passenger callback / pickup retry broken'}
foreach($entry in @(106698,106644)){$v=$sai["${entry}:0"];if($v[4] -ne '1' -or $v[6] -ne '0' -or [int]$v[9] -le 0){throw 'OOC retry is missing'}}
if($sai['106644:1'][4] -ne '1' -or $sai['106644:1'][12] -ne '92' -or $sai['106644:1'][13] -ne '50917' -or $sai['106644:1'][15] -ne '18'){throw 'Absent player introduction credit has no recovery'}
if($sai['106698:4'][4] -ne '4' -or $sai['106698:4'][11] -ne '92' -or $sai['106698:4'][13] -ne '212497'){throw 'Aggro does not interrupt ritual'}
if($tal.Count -ne 6 -or $tal[0][5] -ne '211701' -or [int]$tal[1][2] -lt 2000 -or $tal[2][5] -ne '211704' -or $tal[3][5] -ne '211705' -or [int]$tal[4][2] -lt 100 -or [int]$tal[5][2] -lt 2000){throw 'Native summon tick/teleport lifecycle shortened'}
if($rows -notcontains "OTHER`t0"){throw 'Finale actor changed'}
if($VerifyReleaseBinary){$binary=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe"));foreach($name in @('spell_dark_whispers_ride_skull','spell_dark_whispers_despawn_skull','aura_dark_whispers_ritual_beam')){if(!$binary.Contains($name)){throw "Canonical Release lacks $name"}}}
'PASS: repeat SQL, voice lengths, actual-boarded cleanup, pickup phase retries, OOC attack/ritual events and timed native summon/departure chain. Client animation/audio still pending.'
