$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/addie-hunting"
New-Item -ItemType Directory -Force $out | Out-Null
function Block([string]$source,[string]$marker){
 $a=$source.IndexOf($marker);if($a -lt 0){throw "Missing $marker"};$b=$source.IndexOf('{',$a)+1;$depth=1
 while($depth -and $b -lt $source.Length){if($source[$b] -eq '{'){$depth++};if($source[$b] -eq '}'){$depth--};$b++}
 $source.Substring($a,$b-$a)
}
$source=Get-Content "$repo/src/server/scripts/Legion/highmountain_hunting.h" -Raw
[IO.File]::WriteAllText("$out/Hunting.inc",(Block $source 'namespace AddieHunting'))
$a=$source.IndexOf('class aura_highmountain_addie_hunt');$aura=$source.Substring($a)
foreach($method in @('void Proc(','void Update(uint32')){[IO.File]::WriteAllText("$out/$(if($method -like '*Proc*'){'Proc'}else{'Update'}).inc",(Block $aura $method))}
[IO.File]::WriteAllText("$out/Credit.inc",(Block $source 'void Credit(SpellEffIndex)'))
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$player.IndexOf('        if (qInfo->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT)',$player.IndexOf('bool Player::CanCompleteQuest('))
$b=$player.IndexOf('        if (q_status->Status == QUEST_STATUS_INCOMPLETE)',$a)
[IO.File]::WriteAllText("$out/EventGate.inc",$player.Substring($a,$b-$a))
$a=$player.IndexOf('        if (questid == 40216 && (entry == 97441');$b=$player.IndexOf('// Lightning Rod',$a)
[IO.File]::WriteAllText("$out/GenericCredit.inc",$player.Substring($a,$b-$a))
$a=$unit.IndexOf('    if (isPet() || isTotem())',$unit.IndexOf('// Do KILL and KILLED procs.'))
$b=$unit.IndexOf('    if (victim->GetCreatureType()', $a)
[IO.File]::WriteAllText("$out/PetKill.inc",$unit.Substring($a,$b-$a))
$mgr=Get-Content "$repo/src/server/game/Spells/SpellMgr.cpp" -Raw
$a=$mgr.IndexOf('        if (spell->GetAuraOptions()->ProcTypeMask == 0)',$mgr.IndexOf('void SpellMgr::LoadSpellProcEvents()'))
$b=$mgr.IndexOf('        ++count;',$a)
[IO.File]::WriteAllText("$out/ProcAdmission.inc",$mgr.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/AddieHuntingTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif %errorlevel% neq 0 exit /b %errorlevel%`n`"$out/test.exe`"`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native Addie hunting progression regression failed'}
$migration=Get-Content "$repo/sql/updates/world/2026_10_22_02_addie_hunting.sql" -Raw
$sql=''
foreach($table in @('quest_template','quest_start_scripts','creature_template','smart_scripts','spell_script_names','spell_proc_event')){
 $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
}
$select=@'
SELECT 'start',StartScript FROM quest_template WHERE ID=40216;
SELECT 'event',SpecialFlags & 2 FROM quest_template_addon WHERE ID=40216;
SELECT 'legacy',COUNT(*) FROM quest_start_scripts WHERE id=40216 AND command=7 AND datalong=40216;
SELECT 'actor',AIName,ScriptName FROM creature_template WHERE entry=99039;
SELECT 'bindings',COUNT(*) FROM spell_script_names WHERE (spell_id=188723 AND ScriptName='aura_highmountain_addie_hunt') OR (spell_id IN(196679,196680,212840) AND ScriptName='spell_highmountain_hunting_skill');
SELECT 'proc',procFlags,CustomChance,effectmask FROM spell_proc_event WHERE entry=188723;
SELECT 'random',COUNT(*) FROM smart_scripts WHERE source_type=0 AND ((entryorguid=99039) OR (entryorguid IN(94151,94198,94238,98808,99481) AND action_type=45 AND target_type=11 AND target_param1=99039));
SELECT 'objectives',StorageIndex,ObjectID,Amount FROM quest_objectives WHERE QuestID=40216 ORDER BY StorageIndex;
'@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@(($sql+"`n"+$migration+"`n"+$select+"`n"+$migration+"`n"+$select) | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Addie migration replay failed'}}finally{$env:MYSQL_PWD=$old}
foreach($line in @("start`t0","event`t2","legacy`t0","actor`t`tnpc_highmountain_addie_hunt","bindings`t4","proc`t2`t100`t1","random`t0","objectives`t0`t97441`t38","objectives`t5`t97441`t56","objectives`t6`t97441`t70")){
 if(@($rows|Where-Object {$_ -eq $line}).Count -ne 2){throw "Addie data mismatch: $line"}
}
'PASS: actual native objective thresholds and twice-applied temporary migration; no character state modified.'
