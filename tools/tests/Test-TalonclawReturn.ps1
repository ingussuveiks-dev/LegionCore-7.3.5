param([switch]$BaselineSql)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/talonclaw-return"
New-Item -ItemType Directory -Force $out | Out-Null
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,$first);if($first -lt 0 -or $last -lt $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing production block $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
[IO.File]::WriteAllText("$out/Return.inc",(Section $source 'namespace TalonclawReturn' 'void AddSC_artifact_acquisition()'))
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
[IO.File]::WriteAllText("$out/ObjectiveLookup.inc",(Section $source 'int32 Player::GetQuestObjectiveData(uint32' 'int32 Player::GetQuestObjectiveData(Quest const*'))
$credit=Section $source 'void Player::KilledMonsterCredit(' 'void Player::KilledPlayerCredit()'
[IO.File]::WriteAllText("$out/CreditLoop.inc",(Block $credit 'for (QuestObjective const& obj : qInfo->GetObjectives())'))
$source=Get-Content "$repo/src/server/game/Quests/QuestDef.h" -Raw
[IO.File]::WriteAllText("$out/ObjectiveFlags.inc",((Block $source 'enum QuestObjectiveFlags')+';'))
[IO.File]::WriteAllText("$out/ObjectiveTypes.inc",((Block $source 'enum QuestObjectiveType')+';'))
$source=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
[IO.File]::WriteAllText("$out/PersonalCredit.inc",(Section $source 'void Spell::EffectKillCreditPersonal(' 'void Spell::EffectKillCredit('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/TalonclawReturnTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native Talonclaw return handler regression failed'}
$migration=if($BaselineSql){''}else{Get-Content "$repo/sql/updates/world/2026_10_23_12_talonclaw_return_credit.sql" -Raw}
$sql=@"
CREATE TEMPORARY TABLE saved_objectives LIKE quest_objectives;
INSERT INTO saved_objectives SELECT * FROM quest_objectives WHERE QuestID=40385;
CREATE TEMPORARY TABLE quest_objectives LIKE saved_objectives;
INSERT INTO quest_objectives SELECT * FROM saved_objectives;
UPDATE quest_objectives SET Amount=0 WHERE ID=280794;
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=99480 AND source_type=0;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
UPDATE smart_scripts SET link=4 WHERE id=3;
DELETE FROM smart_scripts WHERE id=4;
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,event_chance,event_flags,action_type,action_param1,target_type,target_param1,comment)
VALUES(99480,0,4,6,100,1,33,103798,21,50,'Baseline premature return credit');
$migration
$migration
SELECT 'claim',Amount,Flags,StorageIndex FROM quest_objectives WHERE ID=280794;
SELECT 'other-objectives',COUNT(*) FROM quest_objectives q JOIN saved_objectives s ON q.ID=s.ID WHERE q.ID<>280794 AND q.Amount=s.Amount AND q.Flags=s.Flags AND q.ObjectID=s.ObjectID;
SELECT 'root',COUNT(*) FROM smart_scripts WHERE id=1 AND link=2 AND event_type=6 AND action_type=205 AND action_param1=92 AND action_param2=47688;
SELECT 'links',COUNT(*) FROM smart_scripts WHERE (id=2 AND link=3 AND event_type=61 AND action_type=12 AND action_param1=100310) OR (id=3 AND link=0 AND event_type=61 AND action_type=50 AND action_param1=248834);
SELECT 'early-credit',COUNT(*) FROM smart_scripts WHERE id=4;
DELETE FROM smart_scripts WHERE id=4;
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,event_chance,event_flags,action_type,action_param1,target_type,comment)
VALUES(99480,0,4,6,100,1,33,999999,21,'Custom unrelated credit');
UPDATE smart_scripts SET link=4 WHERE id=3;
UPDATE quest_objectives SET Amount=3 WHERE ID=280794;
$migration
SELECT 'custom-credit',COUNT(*) FROM smart_scripts WHERE id=4 AND action_param1=999999;
SELECT 'custom-link',COUNT(*) FROM smart_scripts WHERE id=3 AND link=4;
SELECT 'custom-amount',Amount FROM quest_objectives WHERE ID=280794;
UPDATE smart_scripts SET action_param1=103798 WHERE id=4;
UPDATE smart_scripts SET link=7 WHERE id=3;
$migration
SELECT 'custom-chain',COUNT(*) FROM smart_scripts WHERE (id=3 AND link=7) OR (id=4 AND action_param1=103798);
"@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$connection[3]
 $rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Return temporary SQL fixture failed'}
}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("claim`t1`t2`t1","other-objectives`t2","root`t1","links`t2","early-credit`t0","custom-credit`t1","custom-link`t1","custom-amount`t3","custom-chain`t2")){
 if($rows -notcontains $expected){throw "Talonclaw return fixture mismatch: $expected"}
}
'PASS: actual-schema repeated return migration persists claim1, removes only the premature native death credit/bad LINK, preserves boss/chest actions and custom rows'
