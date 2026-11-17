param([switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/niskara-independent-combat"
function Section([string]$s,[string]$a,[string]$b){$i=$s.IndexOf($a);$j=$s.IndexOf($b,[Math]::Max(0,$i));if($i -lt 0 -or $j -le $i){throw "Missing production $a"};$s.Substring($i,$j-$i)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $config=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $migration=Get-Content "$repo/sql/updates/world/2026_11_17_10_niskara_independent_combat_events.sql" -Raw
 $prior=$env:MYSQL_PWD
 try {
  $env:MYSQL_PWD=$config[3]
  $columns=@('SHOW COLUMNS FROM smart_scripts;'|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($config[0]) --port=$($config[1]) --user=$($config[2]) --database=$($config[4]) --batch --raw --skip-column-names|ForEach-Object {($_ -split "`t")[0]})
  if($LASTEXITCODE -or !$columns.Count){throw 'Native schema query failed'}
  $same=(@($columns|Where-Object {$_ -ne 'link'}|ForEach-Object {"a.``$_``<=>b.``$_``"}) -join ' AND ')
  $roots='((entryorguid=109357 AND id=0) OR (entryorguid IN(109108,109109) AND id=6))'
  $rows='entryorguid,id,link,event_type,action_type,action_param1,event_param1,event_param2,event_param3,event_param4'
  $reset='DELETE FROM smart_scripts;INSERT INTO smart_scripts SELECT * FROM saved_niskara_smart;'
  $sql=@"
CREATE TEMPORARY TABLE saved_niskara_smart LIKE smart_scripts;
INSERT INTO saved_niskara_smart SELECT * FROM smart_scripts WHERE source_type=0 AND entryorguid IN(109357,109108,109109);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_niskara_smart;
INSERT INTO smart_scripts SELECT * FROM saved_niskara_smart;
CREATE TEMPORARY TABLE saved_niskara_actors LIKE creature_template;
INSERT INTO saved_niskara_actors SELECT * FROM creature_template WHERE entry IN(109357,109108,109109);
CREATE TEMPORARY TABLE creature_template LIKE saved_niskara_actors;
INSERT INTO creature_template SELECT * FROM saved_niskara_actors;
-- Pinned old bad references, also after the actual migration is installed.
UPDATE smart_scripts SET link=CASE WHEN entryorguid=109357 THEN 1 ELSE 7 END WHERE $roots;
SELECT 'B',$rows FROM smart_scripts ORDER BY entryorguid,id;
$migration
$migration
SELECT 'A',$rows FROM smart_scripts ORDER BY entryorguid,id;
SELECT 'bad-links',COUNT(*) FROM smart_scripts a LEFT JOIN smart_scripts b ON a.entryorguid=b.entryorguid AND a.source_type=b.source_type AND a.link=b.id AND b.event_type=61 WHERE a.link<>0 AND b.id IS NULL;
SELECT 'rows',COUNT(*) FROM saved_niskara_smart;
SELECT 'fields',COUNT(*) FROM smart_scripts a JOIN saved_niskara_smart b USING(entryorguid,source_type,id) WHERE $same;
SELECT 'timers',COUNT(*) FROM smart_scripts WHERE event_type=0 AND event_param1=3000 AND event_param2=5000 AND event_param3=4000 AND event_param4=6000 AND action_type=11 AND target_type=2;
$reset
UPDATE smart_scripts SET link=CASE WHEN entryorguid=109357 THEN 1 ELSE 7 END WHERE $roots;
UPDATE creature_template SET ScriptName='custom';
$migration
SELECT 'custom-actor',COUNT(*) FROM smart_scripts WHERE $roots AND link<>0;
UPDATE creature_template SET ScriptName='';
UPDATE smart_scripts SET event_type=61 WHERE (entryorguid=109357 AND id=1) OR (entryorguid IN(109108,109109) AND id=7);
$migration
SELECT 'valid-link-target',COUNT(*) FROM smart_scripts WHERE $roots AND link<>0;
UPDATE smart_scripts SET event_type=0 WHERE (entryorguid=109357 AND id=1) OR (entryorguid IN(109108,109109) AND id=7);
UPDATE smart_scripts SET event_chance=50 WHERE $roots;
$migration
SELECT 'custom-root',COUNT(*) FROM smart_scripts WHERE $roots AND link<>0;
SELECT 'EOF';
"@
  [IO.File]::WriteAllText("$out/fixture.sql",$sql)
  $exit=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($config[0])","--port=$($config[1])","--user=$($config[2])","--database=$($config[4])",'--batch','--raw','--skip-column-names') -InputFile "$out/fixture.sql" -OutputFile "$out/fixture.stdout" -ErrorFile "$out/fixture.stderr"
  if($exit -or (Get-Item "$out/fixture.stderr").Length){throw 'Niskara temporary SQL regression failed'}
  $values=@(Get-Content "$out/fixture.stdout")
  if($values[-1] -ne 'EOF'){throw 'SQL output was incomplete'}
  foreach($expected in @("bad-links`t0","timers`t3","custom-actor`t3","valid-link-target`t3","custom-root`t3")){if($values -notcontains $expected){throw "SQL outcome differs: $expected"}}
  if((($values|Where-Object {$_ -like "rows`t*"}) -replace '^rows','') -ne (($values|Where-Object {$_ -like "fields`t*"}) -replace '^fields','')){throw 'Other native graph fields changed'}
  foreach($version in @('B','A')){[IO.File]::WriteAllLines("$out/$version.tsv",[string[]]@($values|Where-Object {$_ -like "$version`t*"}|ForEach-Object {$_.Substring(2)}))}
  if($VerifyLiveData){
   $live=@("SELECT 'live',COUNT(*) FROM smart_scripts WHERE source_type=0 AND $roots AND link=0;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($config[0]) --port=$($config[1]) --user=$($config[2]) --database=$($config[4]) --batch --raw --skip-column-names)
   if($LASTEXITCODE -or $live -notcontains "live`t3"){throw 'Live repaired roots differ'}
  }
 } finally {$env:MYSQL_PWD=$prior}
 $source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 [IO.File]::WriteAllText("$out/FindLinkedEvent.inc",(Section $source 'SmartScriptHolder SmartScript::FindLinkedEvent(' 'void SmartScript::OnUpdate('))
 [IO.File]::WriteAllText("$out/LinkTail.inc",(Section $source '    if (e.link && e.link != e.event_id)' 'void SmartScript::InstallTemplate('))
 [IO.File]::WriteAllText("$out/CombatDispatch.inc",(Section $source '        case SMART_EVENT_UPDATE_IC:' '        case SMART_EVENT_HEALTH_PCT:'))
 $init=Section $source 'void SmartScript::InitTimer(' 'void SmartScript::RecalcTimer('
 [IO.File]::WriteAllText("$out/CombatTimer.inc",(Section $init 'case SMART_EVENT_UPDATE:' 'case SMART_EVENT_DISTANCE_CREATURE:'))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/NiskaraIndependentCombatTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/B.tsv`" `"$out/A.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native independent combat execution failed'}
 'PASS idempotent three-row repair, complete native graph field/timer preservation, foreign actor/root/valid-link refusal; no live records changed by fixture'
}
