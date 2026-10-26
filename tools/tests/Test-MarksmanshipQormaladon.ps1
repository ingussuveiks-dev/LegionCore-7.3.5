param([switch]$AddressSanitizer,[switch]$VerifyNativeCatalog,[string]$MigrationFile)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-qormaladon"
if(!$MigrationFile){$MigrationFile="$repo/sql/updates/world/2026_10_26_06_marksmanship_qormaladon_admission.sql"}
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$repo/build-extractors/runtime-test/legion-legendary-catalog","$repo/build-extractors/runtime-test/artifact-acquisition") -Test {
 $source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/artifact_acquisition.cpp" -Raw
 $first=$source.IndexOf('namespace MarksmanshipInquisitor');$last=$source.IndexOf('namespace TalonclawDakarr',$first)
 [IO.File]::WriteAllText("$out/Qormaladon.inc",$source.Substring($first,$last-$first))
 foreach($token in @('RegisterCreatureAI(npc_marksmanship_qormaladon);','new condition_marksmanship_qormaladon_ready();')){if(!$source.Contains($token)){throw "Missing registration $token"}}
 $source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
 foreach($name in @('SetData','DamageTaken','JustDied')){[IO.File]::WriteAllText("$out/$name.inc",(Block $source "void SmartAI::$name("))}
 $source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
 [IO.File]::WriteAllText("$out/Conditions.inc",(Block $source 'case CONDITION_SCENARION_STEP:')+(Block $source 'case CONDITION_OBJECT_ENTRY:'))
 $source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 [IO.File]::WriteAllText("$out/Owner.inc",(Block $source 'Player* GetScriptCreditPlayer('))
 [IO.File]::WriteAllText("$out/Actions.inc",(Block $source 'case SMART_ACTION_REMOVE_UNIT_FLAG:')+(Block $source 'case SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA:'))
 $context='ConditionSourceInfo info = ConditionSourceInfo(unit ? unit : GetBaseObject(), GetBaseObject());'
 if(!$source.Contains($context)){throw 'SmartAI condition context changed'}
 [IO.File]::WriteAllText("$out/Context.inc",$context)
 $migration=Get-Content -LiteralPath $MigrationFile -Raw
 $filters=@{creature_template='entry=101269';creature='id=101269';smart_scripts='entryorguid=101269 AND source_type=0';conditions='SourceTypeOrReferenceId=22 AND SourceEntry=101269'}
 $tables=@('creature_template','creature','smart_scripts','conditions')
 $sql=($tables|ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_;INSERT INTO saved_$_ SELECT * FROM $_ WHERE $($filters[$_]);CREATE TEMPORARY TABLE $_ LIKE saved_$_;INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 # Only TEMP shadows reconstruct the recognized pre-repair state after installation.
 $baseline=@"
UPDATE creature_template SET ScriptName='' WHERE entry=101269 AND ScriptName='npc_marksmanship_qormaladon';
DELETE FROM conditions WHERE SourceGroup IN(2,11) AND SourceId=0 AND ((ConditionTypeOrReference=44 AND ConditionTarget=1 AND ConditionValue1=972 AND ConditionValue2=5 AND ScriptName='') OR ScriptName='condition_marksmanship_qormaladon_ready');
"@
 $sql+="`n$baseline`n$migration`n$migration`n"+@"
SELECT 'binding',COUNT(*) FROM creature_template WHERE entry=101269 AND AIName='SmartAI' AND ScriptName='npc_marksmanship_qormaladon' AND unit_flags=256;
SELECT 'gate-count',COUNT(*) FROM conditions WHERE SourceGroup IN(2,11) AND SourceId=0;
SELECT 'gate',SourceGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ScriptName FROM conditions WHERE SourceGroup IN(2,11) AND SourceId=0 ORDER BY SourceGroup,ConditionTypeOrReference;
SELECT 'smart-before',smart_scripts.* FROM saved_smart_scripts smart_scripts ORDER BY id,link;
SELECT 'smart-after',smart_scripts.* FROM smart_scripts ORDER BY id,link;
SELECT 'other-before',conditions.* FROM saved_conditions conditions WHERE SourceGroup NOT IN(2,11) ORDER BY SourceGroup,ElseGroup,ConditionTypeOrReference;
SELECT 'other-after',conditions.* FROM conditions WHERE SourceGroup NOT IN(2,11) ORDER BY SourceGroup,ElseGroup,ConditionTypeOrReference;
SELECT 'row',id,link,event_type,event_flags,event_param1,event_param2,action_type,action_param1,action_param2,target_type FROM smart_scripts WHERE id IN(1,10,11,12) ORDER BY id;
"@
 $reset=($tables|ForEach-Object {"DELETE FROM $_;INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $variants=@{
  'custom-script'="UPDATE creature_template SET ScriptName='custom_actor' WHERE entry=101269;"
  'custom-ai'="UPDATE creature_template SET AIName='custom_ai' WHERE entry=101269;"
  'custom-immunity'='UPDATE creature_template SET unit_flags=0 WHERE entry=101269;'
  'custom-spawn'="UPDATE creature SET PhaseId='5587' WHERE guid=370739;"
  'custom-unlock'='UPDATE smart_scripts SET action_param1=2 WHERE id=1;'
  'custom-death'='UPDATE smart_scripts SET target_type=21 WHERE id=10;'
  'custom-cleanup'='UPDATE smart_scripts SET action_param2=11 WHERE id=12;'
  'custom-gate'="INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1) VALUES(22,11,101269,0,1,4,1,1489);"
 }
 foreach($name in $variants.Keys){$sql+="`n$reset`n$baseline`n$($variants[$name])`n$migration`nSELECT '$name',COUNT(*) FROM creature_template WHERE ScriptName='npc_marksmanship_qormaladon';`n"}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Qormaladon TEMP migration failed'}}finally{$env:MYSQL_PWD=$previous}
 foreach($expected in @("binding`t1","gate-count`t3")+@($variants.Keys|ForEach-Object {"$_`t0"})){if($rows -notcontains $expected){throw "Qormaladon SQL mismatch $expected"}}
 foreach($kind in @('smart','other')){
  $before=@($rows|Where-Object {$_ -like "$kind-before`t*"}|ForEach-Object {($_ -split "`t",2)[1]})
  $after=@($rows|Where-Object {$_ -like "$kind-after`t*"}|ForEach-Object {($_ -split "`t",2)[1]})
  if(Compare-Object $before $after){throw "Changed unrelated Qormaladon $kind fields"}
 }
 [IO.File]::WriteAllLines("$out/gates.tsv",[string[]]@($rows|Where-Object {$_ -like "gate`t*"}|ForEach-Object {($_ -split "`t",2)[1]}))
 [IO.File]::WriteAllLines("$out/rows.tsv",[string[]]@($rows|Where-Object {$_ -like "row`t*"}|ForEach-Object {($_ -split "`t",2)[1]}))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $definition=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/MarksmanshipQormaladonTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Qormaladon native regression failed'}
 if($VerifyNativeCatalog){
  $previous=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$PSScriptRoot/Test-SurvivalArtifactCatalog.ps1" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Matching native catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
  $catalog=Get-Content "$repo/build-extractors/runtime-test/artifact-acquisition/survival-native.tsv"
  foreach($expected in @("MARKSMANSHIP_CRITERION`t5`t46665`t29457`t92`t49654`t0","MARKSMANSHIP_RULE`t29457`t0`t0`t0`t0`t0`t0")){if($catalog -notcontains $expected){throw 'Native Qormaladon stage/event changed'}}
 }
 'PASS Qormaladon native wrappers/conditions/actions, TEMP migration repeated/custom guards and preserved complete SmartAI rows; modeled event boundaries are separate from live client traversal'
}
