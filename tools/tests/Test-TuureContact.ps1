param([switch]$AddressSanitizer,[switch]$VerifyNativeCatalog,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/tuure-contact"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/diagnostics" -RuntimeDirectories @($repo,$out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
 $effects=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
 $smart=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 [IO.File]::WriteAllText("$out/NativeClick.inc",(Block $unit 'bool Unit::HandleSpellClick('))
 [IO.File]::WriteAllText("$out/NativeOnClick.inc",(Block (Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw) 'void SmartAI::OnSpellClick('))
 [IO.File]::WriteAllText("$out/NativePersonal.inc",(Block $effects 'void Spell::EffectKillCreditPersonal('))
 [IO.File]::WriteAllText("$out/NativeTaxi.inc",(Block $effects 'void Spell::EffectSendTaxi('))
 [IO.File]::WriteAllText("$out/NativeMissingLink.inc",(Block $smart 'if (e.link && e.link != e.event_id)'))
 $credit='targetPlayer->RewardPlayerAndGroupAtEvent(e.action.killedMonster.creature, targetPlayer);'
 if(!$smart.Contains($credit)){throw 'Native action33 target credit changed'}
 [IO.File]::WriteAllText("$out/NativeGroupCredit.inc",$credit)
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $flags=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 $cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`ncl /nologo /EHsc /std:c++17 /W4 /MD $flags /I`"$out`" `"$repo/tools/tests/TuureContactTest.cpp`" /FeTuureContactTest.exe /FoTuureContactTest.obj`nif errorlevel 1 exit /b %errorlevel%`nTuureContactTest.exe`nexit /b %errorlevel%`n"
 [IO.File]::WriteAllText("$out/test.cmd",$cmd);& "$out/test.cmd";if($LASTEXITCODE){throw 'Native contact regression failed'}
 $migration=Get-Content "$repo/sql/updates/world/2026_10_27_01_tuure_contact_native_credit.sql" -Raw
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 function Query([string]$sql,[switch]$ExpectFailure){
  $previous=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$db[3];$result=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names 2> "$out/sql-stderr.log");$code=$LASTEXITCODE}finally{$env:MYSQL_PWD=$previous}
  if($ExpectFailure){if(!$code -or (Get-Content "$out/sql-stderr.log" -Raw) -notmatch 'Tuure contact custom/missing native metadata'){throw 'Expected custom-data refusal missing'};return}
  if($code){throw 'Contact TEMP/live read-only query failed; inspect captured stderr'};$result
 }
 $setup=@'
CREATE TEMPORARY TABLE saved_sai LIKE smart_scripts;
INSERT INTO saved_sai SELECT * FROM smart_scripts WHERE entryorguid=105697;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_sai;
INSERT INTO smart_scripts SELECT * FROM saved_sai;
CREATE TEMPORARY TABLE saved_template LIKE creature_template;
INSERT INTO saved_template SELECT * FROM creature_template WHERE entry=105697;
CREATE TEMPORARY TABLE creature_template LIKE saved_template;
INSERT INTO creature_template SELECT * FROM saved_template;
CREATE TEMPORARY TABLE saved_click LIKE npc_spellclick_spells;
INSERT INTO saved_click SELECT * FROM npc_spellclick_spells WHERE npc_entry=105697;
CREATE TEMPORARY TABLE npc_spellclick_spells LIKE saved_click;
INSERT INTO npc_spellclick_spells SELECT * FROM saved_click;
DELETE FROM smart_scripts WHERE entryorguid=105697 AND source_type=0;
INSERT INTO smart_scripts(entryorguid,source_type,id,link,event_type,action_type,action_param1,target_type,comment) VALUES(105697,0,0,1,73,33,105697,7,'Spell Click - Kill Credit'),(105698,0,0,1,73,33,105698,7,'unrelated');
'@
 $rows=Query ($setup+$migration+$migration+@'
SELECT 'actor',COUNT(*) FROM smart_scripts WHERE entryorguid=105697 AND source_type=0;
SELECT 'other',COUNT(*) FROM smart_scripts WHERE entryorguid=105698;
SELECT 'native-click',npc_entry,spell_id,cast_flags,user_type,add_npc_flag FROM npc_spellclick_spells;
SELECT 'ai',AIName,ScriptName FROM creature_template;
'@)
 if(($rows -join '|') -ne "actor`t0|other`t1|native-click`t105697`t209441`t1`t0`t1|ai`tSmartAI`t"){throw 'Contact repeat migration or preservation failed'}
 foreach($change in @("UPDATE smart_scripts SET target_type=1 WHERE entryorguid=105697;","UPDATE smart_scripts SET action_param2=1 WHERE entryorguid=105697;","INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,action_type,comment) VALUES(105697,0,1,61,41,'custom link');","UPDATE creature_template SET ScriptName='custom' WHERE entry=105697;","UPDATE npc_spellclick_spells SET spell_id=1 WHERE npc_entry=105697;","DELETE FROM npc_spellclick_spells WHERE npc_entry=105697;")) {Query ($setup+$change+$migration) -ExpectFailure}
 'PASS TEMP migration twice, native click/AI and unrelated actor preserved; six custom/missing-data refusals. No live data modified.'
 if($VerifyNativeCatalog){
  $previous=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TuureContactCatalog.cpp -ProbeName TuureContactCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
  $native=@(Get-Content "$out/native.tsv")
  foreach($row in @("PATH`t5626`t1877`t1877`t0","EFFECT`t0`t6`t0`t1`t0","EFFECT`t1`t90`t105697`t1`t0","EFFECT`t2`t123`t5626`t1`t0")){if($native -notcontains $row){throw 'Native contact path/effect contract differs'}}
  if(@($native|Where-Object {$_ -like "POINT`t*"}).Count -ne 9){throw 'Native contact path point count differs'}
 }
 if($VerifyLiveData){
  $rows=Query "SET SESSION TRANSACTION READ ONLY;SELECT 'sai',COUNT(*) FROM smart_scripts WHERE entryorguid=105697 AND source_type=0;SELECT 'native',npc_entry,spell_id,cast_flags,user_type,add_npc_flag FROM npc_spellclick_spells WHERE npc_entry=105697;"
  if(($rows -join '|') -ne "sai`t0|native`t105697`t209441`t1`t0`t1"){throw 'Live contact migration not applied or data differs'}
 }
}
