param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stromkar-objectives"
New-Item -ItemType Directory -Force $out|Out-Null
$migrationPath="$repo/sql/updates/world/2026_10_24_54_stromkar_objective_slots.sql"
$migration=Get-Content $migrationPath -Raw
$hash=(Get-FileHash $migrationPath -Algorithm SHA1).Hash
$archive="$repo/.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql"
if((Get-FileHash $archive -Algorithm SHA256).Hash -ne '786D8B7F84FE1DAF765B770483721AE529F3D758DA2271F812C261D914F1E99E'){throw 'Pinned TDB735 archive changed'}
$python='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$native=@'
import re,sys
from pathlib import Path
columns=[];inside=False;found={}
for line in Path(sys.argv[1]).open(encoding='utf8'):
 if line.startswith('CREATE TABLE `quest_objectives`'): inside=True;continue
 if inside:
  if line.startswith(')'): inside=False
  else:
   match=re.match(r'  `([^`]+)`',line)
   if match: columns.append(match[1])
 if not line.startswith('INSERT INTO `quest_objectives`'): continue
 for record in re.finditer(r"\((?:'(?:\\.|''|[^'\\])*'|[^'()])*\)",line):
  row=dict(zip(columns,re.findall(r"'(?:\\.|''|[^'\\])*'|[^,]+",record[0][1:-1])))
  if row['QuestID']=='41105':
   found[int(row['ID'])]=tuple(int(row[field]) for field in ('Order','StorageIndex','ObjectID','Flags','VerifiedBuild'))
expected={281862:(0,0,97377,4,25549),286343:(1,3,102394,4,25549),286344:(2,4,113105,0,25549),281863:(3,1,103114,2,25549),282354:(4,2,103739,2,25549)}
assert found==expected,found
for identity,values in sorted(found.items(),key=lambda pair:pair[1][0]): print(identity,*values,sep='\t')
'@ | & $python - $archive
if($LASTEXITCODE){throw 'Native TDB objective probe failed'}
[IO.File]::WriteAllLines("$out/tdb735.tsv",[string[]]$native)
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Objective TEMP read failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$poi=Query 'SELECT DISTINCT ObjectiveIndex,QuestObjectiveID,QuestObjectID FROM quest_poi WHERE QuestID=41105 AND QuestObjectiveID<>0 ORDER BY ObjectiveIndex;'
if(Compare-Object @("1`t286343`t102394","2`t286344`t113105","3`t281863`t103114","4`t282354`t103739") $poi){throw 'Live quest POI story mapping differs from original735'}
$reset='DELETE FROM quest_objectives;INSERT INTO quest_objectives SELECT * FROM saved_objectives;'
$columns='ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2,TaskStep,Bugged'
$sql=@"
CREATE TEMPORARY TABLE saved_objectives LIKE quest_objectives;INSERT INTO saved_objectives SELECT * FROM quest_objectives WHERE QuestID=41105;
UPDATE saved_objectives SET StorageIndex=CASE ID WHEN 286344 THEN 1 WHEN 281863 THEN 2 WHEN 282354 THEN 3 ELSE StorageIndex END
WHERE QuestID=41105 AND ID IN(286344,281863,282354) AND EXISTS(SELECT 1 FROM updates WHERE name='2026_10_24_54_stromkar_objective_slots.sql' AND hash='$hash' AND state='RELEASED');
CREATE TEMPORARY TABLE quest_objectives LIKE saved_objectives;
$reset
SELECT 'before',quest_objectives.* FROM quest_objectives ORDER BY ID;
SELECT 'baseline',$columns FROM quest_objectives WHERE QuestID=41105 ORDER BY StorageIndex,ID;
$migration
SELECT 'first',quest_objectives.* FROM quest_objectives ORDER BY ID;
SELECT 'fixture',$columns FROM quest_objectives WHERE QuestID=41105 ORDER BY StorageIndex,ID;
$migration
SELECT 'second',quest_objectives.* FROM quest_objectives ORDER BY ID;
"@
$variants=[ordered]@{
 'custom-slot'='UPDATE quest_objectives SET StorageIndex=6 WHERE ID=281863;'
 'custom-object'='UPDATE quest_objectives SET ObjectID=999 WHERE ID=282354;'
 'custom-amount'='UPDATE quest_objectives SET Amount=2 WHERE ID=286344;'
 'custom-flags'='UPDATE quest_objectives SET Flags=0 WHERE ID=286343;'
 'custom-flags2'='UPDATE quest_objectives SET Flags2=1 WHERE ID=281862;'
 'custom-type'='UPDATE quest_objectives SET Type=1 WHERE ID=281863;'
 'custom-step'='UPDATE quest_objectives SET TaskStep=1 WHERE ID=281863;'
 'custom-bugged'='UPDATE quest_objectives SET Bugged=1 WHERE ID=281863;'
 'missing-row'='DELETE FROM quest_objectives WHERE ID=286343;'
 'extra-row'='INSERT INTO quest_objectives(ID,QuestID,Type,StorageIndex,ObjectID,Amount)VALUES(999999,41105,0,6,999,1);'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',quest_objectives.* FROM quest_objectives ORDER BY ID;`n$migration`nSELECT '$name-after',quest_objectives.* FROM quest_objectives ORDER BY ID;`n"}
$rows=Query $sql
[IO.File]::WriteAllLines("$out/temp.tsv",[string[]]$rows)
function Snapshot([string]$label){[string[]]@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
function Signature([string]$label){[string]::Join("`n",[string[]](Snapshot $label))}
if((Signature 'first') -cne (Signature 'second')){throw 'Objective migration is not idempotent'}
foreach($name in $variants.Keys){if((Signature "$name-before") -cne (Signature "$name-after")){throw "Custom objectives overwritten: $name"}}
$expected=@("281862`t41105`t0`t0`t97377`t1`t4`t0`t0`t0","281863`t41105`t0`t1`t103114`t1`t2`t0`t0`t0","282354`t41105`t0`t2`t103739`t1`t2`t0`t0`t0","286343`t41105`t0`t3`t102394`t1`t4`t0`t0`t0","286344`t41105`t0`t4`t113105`t1`t0`t0`t0`t0")
if(Compare-Object $expected (Snapshot 'fixture')){throw 'Corrected objectives differ from original735 identity/slots/flags'}
$before=Snapshot 'before';$after=Snapshot 'first'
foreach($index in 0..4){$old=$before[$index].Split("`t");$fixed=$after[$index].Split("`t");$old[3]=$fixed[3];if([string]::Join("`t",$old) -cne $after[$index]){throw 'Non-slot metadata changed'}}
[IO.File]::WriteAllLines("$out/baseline.tsv",[string[]](Snapshot 'baseline'))
[IO.File]::WriteAllLines("$out/fixture.tsv",[string[]](Snapshot 'fixture'))
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$packet=Get-Content "$repo/src/server/game/Entities/Creature/GossipDef.cpp" -Raw
[IO.File]::WriteAllText("$out/QueryObjectives.inc",(Block $packet '    for (QuestObjective const& questObjective : quest->GetObjectives())'))
$packet=Get-Content "$repo/src/server/game/Server/Packets/QuestPackets.cpp" -Raw
[IO.File]::WriteAllText("$out/WriteObjectives.inc",(Block $packet '        for (QuestObjective const& questObjective : Info.Objectives)'))
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$blocks=@('int32 Player::GetQuestObjectiveData(uint32','int32 Player::GetQuestObjectiveData(Quest const*','void Player::SetQuestObjectiveData(','bool Player::HasQuestObjectiveComplete(','bool Player::CanCompleteQuest(','void Player::_LoadQuestStatusObjectives(')
[IO.File]::WriteAllText("$out/PlayerObjectives.inc",[string]::Join("`n",[string[]]@($blocks|ForEach-Object {Block $source $_})))
$credit=Block $source 'void Player::KilledMonsterCredit('
[IO.File]::WriteAllText("$out/Credit.inc",(Block $credit '        if (q_status->Status == QUEST_STATUS_INCOMPLETE'))
$save=$source.IndexOf('                // Save objectives')
[IO.File]::WriteAllText("$out/Save.inc",(Block $source.Substring($save) '                    for (QuestObjective const& obj : quest->GetObjectives())'))
$baseline=[string]::Join("`n",[string[]](& git -C $repo show 'c3e09217:src/server/game/Quests/QuestDef.cpp'));if($LASTEXITCODE){throw 'Pinned objective-loader baseline missing'}
$current=Get-Content "$repo/src/server/game/Quests/QuestDef.cpp" -Raw
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($mode in @('baseline','fixed')){
 $definition=if($mode -eq 'baseline'){'/DBASELINE'}else{''}
 [IO.File]::WriteAllText("$out/ObjectiveLoader.inc",(Block $(if($mode -eq 'baseline'){$baseline}else{$current}) 'void Quest::LoadQuestObjective('))
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/StromkarObjectivesTest.cpp`" /Fe`"$out/$mode.exe`" /Fo`"$out/$mode.obj`"`nif errorlevel 1 exit /b 1`n`"$out/$mode.exe`" `"$out/baseline.tsv`" `"$out/fixture.tsv`"`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw "Native objective $mode failed"}
}
if($VerifyReleaseBinary){
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_54_stromkar_objective_slots.sql';SELECT 'fixture',$columns FROM quest_objectives WHERE QuestID=41105 ORDER BY StorageIndex,ID;"
 if($live -notcontains "release`t$hash`tRELEASED"){throw 'Live54 hash mismatch'}
 $actual=@($live|Where-Object {$_ -like "fixture`t*"}|ForEach-Object {$_.Substring(8)})
 if(Compare-Object $expected $actual){throw 'Live54 slots differ from tested definitions'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& $python "$PSScriptRoot/verify_stromkar_objectives_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical objective loader failed'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: independent pinned TDB735 goals/slots/order, guarded54 TEMP/idempotence/10 custom variants and actual native objective storage/completion/save/load checked. Existing character slots must not be silently remapped; live actor/return-credit/client checks remain separate.'
