param([switch]$AddressSanitizer,[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/demon-souls-gem"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw -Encoding UTF8
 $headers=(Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw -Encoding UTF8)+(Get-Content "$repo/src/server/game/Conditions/ConditionMgr.h" -Raw -Encoding UTF8)
 $names=@('SMART_EVENT_LINK','SMART_EVENT_JUST_SUMMONED','SMART_EVENT_DATA_SET','SMART_EVENT_RESET','SMART_EVENT_FLAG_NOT_REPEATABLE','SMART_EVENT_FLAG_EVENT_NON_COMBAT','SMART_EVENT_FLAG_DONT_RESET','SMART_TARGET_SELF','SMART_TARGET_CREATURE_DISTANCE','SMART_TARGET_PLAYER_DISTANCE','SMART_TARGET_OWNER_OR_SUMMONER','SMART_ACTION_SET_DATA','SMART_ACTION_CALL_KILLEDMONSTER','CONDITION_ALIVE')
 $constants=$names|ForEach-Object {$match=[regex]::Match($headers,"$_\s*=\s*(0x[0-9a-fA-F]+|\d+)");if(!$match.Success){throw "Missing enum $_"};"constexpr uint32 $_=$($match.Groups[1].Value);"}
 [IO.File]::WriteAllLines("$out/Constants.inc",[string[]]$constants)
 foreach($pair in @(@('AnyOwner.inc','src/server/game/Entities/Unit/Unit.cpp','Unit* Unit::GetAnyOwner() const'),@('SetData.inc','src/server/game/AI/SmartScripts/SmartAI.cpp','void SmartAI::SetData('),@('Summoned.inc','src/server/game/AI/SmartScripts/SmartAI.cpp','void SmartAI::IsSummonedBy('))){[IO.File]::WriteAllText("$out/$($pair[0])",(Block (Get-Content "$repo/$($pair[1])" -Raw -Encoding UTF8) $pair[2]))}
 [IO.File]::WriteAllText("$out/CreditPlayer.inc",(Block $source 'Player* GetScriptCreditPlayer('))
 [IO.File]::WriteAllText("$out/Reset.inc",(Block $source 'void SmartScript::OnReset('))
 [IO.File]::WriteAllText("$out/AliveCondition.inc",(Block (Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw -Encoding UTF8) 'case CONDITION_ALIVE:'))
 [IO.File]::WriteAllText("$out/Actions.inc",(Block $source 'case SMART_ACTION_SET_DATA:')+(Block $source 'case SMART_ACTION_CALL_KILLEDMONSTER:'))
 $targets=Block $source 'ObjectList* SmartScript::GetTargets('
 $self=$targets.Substring($targets.IndexOf('case SMART_TARGET_SELF:'),$targets.IndexOf('case SMART_TARGET_VICTIM:')-$targets.IndexOf('case SMART_TARGET_SELF:'))
 [IO.File]::WriteAllText("$out/Targets.inc",$self+(Block $targets 'case SMART_TARGET_CREATURE_DISTANCE:')+(Block $targets 'case SMART_TARGET_PLAYER_DISTANCE:')+(Block $targets 'case SMART_TARGET_OWNER_OR_SUMMONER:'))
 $event=Block $source 'void SmartScript::ProcessEvent('
 $start=$event.IndexOf('{')+1
 [IO.File]::WriteAllText("$out/EventGuards.inc",$event.Substring($start,$event.IndexOf('switch (e.GetEventType())')-$start))
 [IO.File]::WriteAllText("$out/DataEvent.inc",(Block $event 'case SMART_EVENT_DATA_SET:'))
 $action=Block $source 'void SmartScript::ProcessAction('; $start=$action.IndexOf('{')+1
 [IO.File]::WriteAllText("$out/ActionPrefix.inc",$action.Substring($start,$action.IndexOf('switch (e.GetActionType())')-$start))
 [IO.File]::WriteAllText("$out/LinkTail.inc",$action.Substring($action.IndexOf('if (e.link && e.link != e.event_id)')))
 $migration=Get-Content "$repo/sql/updates/world/2026_10_26_04_demon_souls_gem.sql" -Raw
 $tables=@('quest_template','quest_objectives','creature_template','smart_scripts','conditions')
 $sql=($tables|ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_;INSERT INTO saved_$_ SELECT * FROM $_;CREATE TEMPORARY TABLE $_ LIKE saved_$_;INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 # Reconstruct the documented pre-fix shape in TEMP shadows so the same native
 # counterfactual remains runnable after the migration is installed.
 $baseline=@"
DELETE FROM smart_scripts WHERE entryorguid=86963 AND source_type=0 AND id=4 AND event_type=61 AND action_type=45 AND target_type=11 AND target_param1=101943;
UPDATE smart_scripts SET link=0 WHERE entryorguid=86963 AND source_type=0 AND id=2 AND link=4;
UPDATE smart_scripts SET target_param2=10 WHERE entryorguid=86963 AND source_type=0 AND id IN (0,1,2) AND action_type=45 AND target_type=11;
UPDATE smart_scripts SET target_type=18,target_param1=7,target_param2=0,target_param3=0,target_param4=0 WHERE entryorguid=86963 AND source_type=0 AND id=3 AND action_type=33 AND target_type=23;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceId=0 AND SourceEntry=101943 AND ConditionTypeOrReference=36 AND ConditionTarget=1 AND NegativeCondition=1 AND Comment='Demon Souls: Felguard soul requires a dead demon, like the three original types';
"@
 $sql+="`n$baseline`n"
 $select="SELECT '{0}',CONCAT('{{',entryorguid,',',id,',',link,',',event_type,',',event_flags,',',event_param1,',',event_param2,',',action_type,',',action_param1,',',action_param2,',',target_type,',',target_param1,',',target_param2,'}},') FROM smart_scripts WHERE source_type=0 AND ((entryorguid=86963) OR (entryorguid IN (90230,90241,93619,101943) AND id IN (0,1))) ORDER BY entryorguid,id;"
 $sql+="`n"+($select -f 'before')+"`n$migration`n$migration`n"+($select -f 'after')+@"
SELECT 'dead-guards',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceId=0 AND SourceEntry IN (90230,90241,93619,101943) AND ConditionTypeOrReference=36 AND ConditionTarget=1 AND NegativeCondition=1;
SELECT 'other-actions',COUNT(*) FROM saved_smart_scripts a LEFT JOIN smart_scripts b ON a.entryorguid=b.entryorguid AND a.source_type=b.source_type AND a.id=b.id AND a.link=b.link WHERE NOT (a.source_type=0 AND a.entryorguid=86963) AND (b.id IS NULL OR a.event_type<>b.event_type OR a.event_flags<>b.event_flags OR a.action_type<>b.action_type OR a.action_param1<>b.action_param1 OR a.target_type<>b.target_type);
"@
 # A separate negative case verifies that a custom binding blocks the whole repair.
 $sql+=($tables|ForEach-Object {"DELETE FROM $_;INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
 $sql+="`n$baseline`nUPDATE creature_template SET ScriptName='custom_demon' WHERE entry=101943;`n$migration`nSELECT 'custom',COUNT(*) FROM smart_scripts WHERE entryorguid=86963 AND source_type=0 AND id=4;"
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$rows=$sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names;if($LASTEXITCODE){throw 'Demon Souls TEMP migration failed'}}finally{$env:MYSQL_PWD=$previous}
 foreach($prefix in @('before','after')){[IO.File]::WriteAllLines("$out/$(if($prefix -eq 'before'){'Before'}else{'After'})Rows.inc",[string[]]@($rows|Where-Object {$_ -like "$prefix`t*"}|ForEach-Object {$_.Split("`t")[1]}))}
 $counts=@($rows|Where-Object {$_ -notmatch '^(before|after)\t'})
 [IO.File]::WriteAllLines("$out/sql-checks.tsv",[string[]]$counts)
 if(Compare-Object @("dead-guards`t4","other-actions`t0","custom`t0") $counts){throw "Demon Souls migration scope/guards differ: $($counts -join '|')"}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $definition=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/DemonSoulsGemTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Demon Souls SmartAI regression failed'}
 if($VerifyNativeCatalog){
  $previous=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile DemonSoulsCatalog.cpp -ProbeName DemonSoulsCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Demon Souls matching catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
  $catalog=Get-Content "$out/native.tsv" -Encoding UTF8
  foreach($expected in @("ITEM`t122100`t178787`t0","EFFECT`t178787`t0`t28`t0`t86963`t2990`t0`t47`t0","DURATION`t28`t5000`t0`t5000")){if($catalog -notcontains $expected){throw 'Native Soul Gem item/summon/lifetime changed'}}
 }
 'PASS production migration twice in TEMP shadows, dead guards/other actions/custom binding preserved; matching native item/catalog remains separate from client capture/visual proof'
}
