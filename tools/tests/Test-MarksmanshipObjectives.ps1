param([switch]$AddressSanitizer,[switch]$VerifyNativeCatalog,[string]$MigrationFile)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/marksmanship-objectives"
$python='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
if(!$MigrationFile){$MigrationFile="$repo/sql/updates/world/2026_10_26_07_marksmanship_objective_slots.sql"}
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @("$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $archive="$repo/.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql"
 if((Get-FileHash $archive).Hash -cne '786D8B7F84FE1DAF765B770483721AE529F3D758DA2271F812C261D914F1E99E'){throw 'Pinned original TDB735 archive differs'}
 & $python "$repo/tools/research/read_tdb735_marksmanship_objectives.py" $archive > "$out/tdb735.tsv";if($LASTEXITCODE){throw 'Original TDB735 objective identity/slot/order mismatch'}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $migration=Get-Content -LiteralPath $MigrationFile -Raw
 $reset='DELETE FROM quest_objectives;INSERT INTO quest_objectives SELECT * FROM saved_objectives;'
 $columns='ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Bugged'
 $sql=@"
CREATE TEMPORARY TABLE saved_objectives LIKE quest_objectives;INSERT INTO saved_objectives SELECT * FROM quest_objectives WHERE QuestID=40419;
SET @known_fixed := (SELECT COUNT(*) FROM saved_objectives)=6 AND (SELECT COUNT(*) FROM saved_objectives WHERE (ID=281309 AND StorageIndex=0) OR (ID=281310 AND StorageIndex=1) OR (ID=281311 AND StorageIndex=2) OR (ID=281574 AND StorageIndex=3) OR (ID=281978 AND StorageIndex=4) OR (ID=281980 AND StorageIndex=5))=6;
UPDATE saved_objectives SET StorageIndex=CASE ID WHEN 281309 THEN 1 WHEN 281310 THEN 2 WHEN 281311 THEN 3 WHEN 281574 THEN 5 ELSE 4 END WHERE @known_fixed;
CREATE TEMPORARY TABLE quest_objectives LIKE saved_objectives;
$reset
SELECT 'before',quest_objectives.* FROM quest_objectives ORDER BY ID;
SELECT 'baseline',$columns FROM quest_objectives ORDER BY StorageIndex,ID;
$migration
SELECT 'first',quest_objectives.* FROM quest_objectives ORDER BY ID;
SELECT 'fixture',$columns FROM quest_objectives ORDER BY StorageIndex,ID;
$migration
SELECT 'second',quest_objectives.* FROM quest_objectives ORDER BY ID;
"@
 $variants=@{'custom-slot'='UPDATE quest_objectives SET StorageIndex=6 WHERE ID=281980;';'custom-type'='UPDATE quest_objectives SET Type=1 WHERE ID=281978;';'custom-object'='UPDATE quest_objectives SET ObjectID=999 WHERE ID=281574;';'custom-amount'='UPDATE quest_objectives SET Amount=2 WHERE ID=281311;';'custom-flags'='UPDATE quest_objectives SET Flags=0 WHERE ID=281980;';'custom-flags2'='UPDATE quest_objectives SET Flags2=1 WHERE ID=281309;';'custom-step'='UPDATE quest_objectives SET TaskStep=1 WHERE ID=281310;';'custom-bugged'='UPDATE quest_objectives SET Bugged=1 WHERE ID=281574;';'missing'='DELETE FROM quest_objectives WHERE ID=281309;';'extra'='INSERT INTO quest_objectives(ID,QuestID,Type,StorageIndex,ObjectID,Amount)VALUES(999999,40419,0,6,999,1);'}
 foreach($name in $variants.Keys){$sql+="`n$reset`n$($variants[$name])`nSELECT '$name-before',quest_objectives.* FROM quest_objectives ORDER BY ID;`n$migration`nSELECT '$name-after',quest_objectives.* FROM quest_objectives ORDER BY ID;"}
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$rows=@($sql|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Objective TEMP migration failed'}}finally{$env:MYSQL_PWD=$previous}
 function Snapshot([string]$label){[string[]]@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
 function Signature([string]$label){[string]::Join("`n",[string[]](Snapshot $label))}
 if((Signature 'first') -cne (Signature 'second')){throw 'Non-idempotent objective migration'}
 foreach($name in $variants.Keys){if((Signature "$name-before") -cne (Signature "$name-after")){throw "Custom definitions overwritten: $name"}}
 $expected=@("281309`t40419`t0`t0`t102128`t1`t0`t0`t0`t0","281310`t40419`t0`t1`t102129`t1`t2`t0`t0`t0","281311`t40419`t0`t2`t102131`t1`t2`t0`t0`t0","281574`t40419`t0`t3`t102548`t1`t2`t0`t0`t0","281978`t40419`t0`t4`t103481`t1`t0`t0`t0`t0","281980`t40419`t0`t5`t103490`t1`t6`t0`t0`t0")
 if(Compare-Object $expected (Snapshot 'fixture')){throw 'Original native slots/retained acceptance credit differ'}
 $before=Snapshot 'before';$after=Snapshot 'first'
 for($index=0;$index -lt 6;++$index){$row=$before[$index].Split("`t");$row[3]=$after[$index].Split("`t")[3];if(($row -join "`t") -cne $after[$index]){throw 'Non-slot objective metadata changed'}}
 [IO.File]::WriteAllLines("$out/baseline.tsv",[string[]](Snapshot 'baseline'));[IO.File]::WriteAllLines("$out/fixture.tsv",[string[]](Snapshot 'fixture'))
 $source=Get-Content "$repo/src/server/game/Quests/QuestDef.cpp" -Raw
 [IO.File]::WriteAllText("$out/ObjectiveLoader.inc",(Block $source 'void Quest::LoadQuestObjective('))
 [IO.File]::WriteAllText("$out/QueryObjectives.inc",(Block (Get-Content "$repo/src/server/game/Entities/Creature/GossipDef.cpp" -Raw) '    for (QuestObjective const& questObjective : quest->GetObjectives())'))
 [IO.File]::WriteAllText("$out/WriteObjectives.inc",(Block (Get-Content "$repo/src/server/game/Server/Packets/QuestPackets.cpp" -Raw) '        for (QuestObjective const& questObjective : Info.Objectives)'))
 $source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
 $blocks=@('int32 Player::GetQuestObjectiveData(uint32','int32 Player::GetQuestObjectiveData(Quest const*','void Player::SetQuestObjectiveData(','bool Player::HasQuestObjectiveComplete(','bool Player::CanCompleteQuest(','void Player::_LoadQuestStatusObjectives(')
 [IO.File]::WriteAllText("$out/PlayerObjectives.inc",[string]::Join("`n",[string[]]@($blocks|ForEach-Object {Block $source $_})))
 [IO.File]::WriteAllText("$out/Credit.inc",(Block (Block $source 'void Player::KilledMonsterCredit(') '        if (q_status->Status == QUEST_STATUS_INCOMPLETE'))
 [IO.File]::WriteAllText("$out/Save.inc",(Block $source.Substring($source.IndexOf('                // Save objectives')) '                    for (QuestObjective const& obj : quest->GetObjectives())'))
 # Reuse the existing controlled service boundary, not a second copy of production logic.
 $hostSource=Get-Content "$PSScriptRoot/StromkarObjectivesTest.cpp" -Raw
 $hostSource=$hostSource.Substring(0,$hostSource.IndexOf('int main(')).Replace('41105','40419').Replace('assert(rows.size()==5)','assert(rows.size()==6)')
 [IO.File]::WriteAllText("$out/ObjectiveHost.inc",$hostSource)
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $definition=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/MarksmanshipObjectivesTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`" `"$out/baseline.tsv`" `"$out/fixture.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native objective regression failed'}
 if($VerifyNativeCatalog){$previous=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile MarksmanshipObjectiveCatalog.cpp -ProbeName MarksmanshipObjectiveCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native objective catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous};$catalog=Get-Content "$out/native.tsv";foreach($expected in @("OBJECTIVE_COUNT`t0`t0","OBJECTIVE_COUNT`t1`t0","EFFECT`t1`t205040`t1`t90`t102548`t0`t25`t0")){if($catalog -notcontains $expected){throw 'Native objective/return evidence differs'}}}
 'PASS native objective storage/order/save/load, original735 source, repeated migration/ten custom variants and unchanged metadata. Existing character progress requires a separate aggregate-zero preflight; no live client verification implied.'
}
