param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/vrekt-world-quest"
New-Item -ItemType Directory -Force $out | Out-Null
function Query([string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Vrekt SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
 $rows
}
$migration=Get-Content "$repo/sql/updates/world/2026_10_18_10_vrekt_world_quest.sql" -Raw
$rows=@(Query @"
CREATE TEMPORARY TABLE saved_updates LIKE world_quest_update;
INSERT INTO saved_updates SELECT * FROM world_quest_update;
CREATE TEMPORARY TABLE world_quest_update LIKE saved_updates;
INSERT INTO world_quest_update SELECT * FROM saved_updates WHERE QuestID<>44923;
SELECT 'missing',COUNT(*) FROM world_quest_update WHERE QuestID=44923;
SELECT 'other-before',u.* FROM world_quest_update u ORDER BY QuestID,Timer,VariableID;
$migration
SELECT 'first',u.* FROM world_quest_update u WHERE QuestID=44923;
$migration
SELECT 'repeat',u.* FROM world_quest_update u WHERE QuestID=44923;
SELECT 'other-after',u.* FROM world_quest_update u WHERE QuestID<>44923 ORDER BY QuestID,Timer,VariableID;
SELECT 'fixture',QuestID,Timer,VariableID,VariableID1,Value,Value1,EventID FROM world_quest_update WHERE QuestID=44923;
UPDATE world_quest_update SET Timer=1234,VariableID=777,Value=9,AreaID='123',EventID=12,VerifiedBuild=42 WHERE QuestID=44923;
SELECT 'external-before',u.* FROM world_quest_update u WHERE QuestID=44923;
$migration
SELECT 'external-after',u.* FROM world_quest_update u WHERE QuestID=44923;
DELETE FROM world_quest_update WHERE QuestID IN(44737,44923);
$migration
SELECT 'no-policy',COUNT(*) FROM world_quest_update WHERE QuestID=44923;
"@)
function Tagged([string]$prefix){@($rows | Where-Object {$_ -like "$prefix`t*"} | ForEach-Object {($_ -split "`t",2)[1]})}
$first=Tagged 'first';$repeat=Tagged 'repeat'
if($first.Count -ne 1 -or ($first -join '|') -ne ($repeat -join '|') -or
 ((Tagged 'other-before') -join '|') -ne ((Tagged 'other-after') -join '|') -or
 ((Tagged 'external-before') -join '|') -ne ((Tagged 'external-after') -join '|') -or
 "missing`t0" -notin $rows -or "no-policy`t0" -notin $rows){throw 'Missing-candidate/repeated/scoped/operator-policy migration check failed'}
$fixture=Tagged 'fixture'
if(($fixture -join '') -ne "44923`t259200`t12678`t0`t3`t0`t0"){throw 'Current server duration/native state mapping changed; re-audit policy'}
$fixture | Set-Content "$out/update.tsv"
if($Live){
 $liveRows=@(Query 'SELECT QuestID,Timer,VariableID,VariableID1,Value,Value1,EventID FROM world_quest_update WHERE QuestID=44923;')
 if(($liveRows -join '|') -ne ($fixture -join '|')){throw 'Live Vrekt candidate differs from tested migration'}
}
$native=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
$stateType=[regex]::Match((Get-Content "$repo/src/server/game/DataStores/DB2Structure.h" -Raw),'typedef std::pair<\w+,\s*\w+> WorldQuestState;')
if(!$stateType.Success){throw 'Missing native world-state pair type'}
[IO.File]::WriteAllText("$out/WorldQuestState.inc",$stateType.Value)
foreach($enum in @(@('src/server/game/Quests/QuestDef.h','QuestInfo'),@('src/server/game/Globals/QuestData.h','WorldQuestResetType'))){
 $definition=Get-Content "$repo/$($enum[0])" -Raw
 $match=[regex]::Match($definition,('(?s)enum '+$enum[1]+'\b.*?\n\};'))
 if(!$match.Success){throw "Missing native enum $($enum[1])"}
 [IO.File]::WriteAllText("$out/$($enum[1]).inc",$match.Value)
}
function Block([string]$source,[string]$signature){
 $a=$source.IndexOf($signature);if($a -lt 0){throw "Missing native boundary $signature"}
 $b=$source.IndexOf('{',$a)+1;$depth=1
 while($depth -and $b -lt $source.Length){if($source[$b] -eq '{'){++$depth};if($source[$b] -eq '}'){--$depth};++$b}
 if($depth){throw 'Unterminated native block'};$source.Substring($a,$b-$a)
}
foreach($pair in @(
 @('WorldQuestTemplate const* QuestDataStoreMgr::GetWorldQuestTemplate(','TemplateLookup'),
 @('WorldQuestUpdate const* QuestDataStoreMgr::GetWorldQuestUpdate(','UpdateLookup'),
 @('WorldQuest const* QuestDataStoreMgr::GenerateNewWorldQuest(','GenerateNew'),
 @('WorldQuest const* QuestDataStoreMgr::GenerateWorldQuest(','Generate'),
 @('bool QuestDataStoreMgr::CanBeActivate(','CanActivate'),
 @('while (countQuest > 0 && !_wQS.empty())','CandidateLoop'))){
 [IO.File]::WriteAllText("$out/$($pair[1]).inc",(Block $native $pair[0]))
}
$baseline=& git -C $repo show e9d85586:src/server/game/Globals/QuestData.cpp
if($LASTEXITCODE){throw 'Pinned pre-fix admission source unavailable'}
$old=Block ($baseline -join "`n") 'bool QuestDataStoreMgr::CanBeActivate('
New-Item -ItemType Directory -Force "$out/baseline" | Out-Null
Copy-Item "$out/*.inc" "$out/baseline/"
[IO.File]::WriteAllText("$out/baseline/CanActivate.inc",$old)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($variant in @('current','baseline')){
 $include=if($variant -eq 'baseline'){"$out/baseline"}else{$out}
 $define=if($variant -eq 'baseline'){'/DOLD_ADMISSION'}else{''}
 $cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 $define /I`"$include`" `"$PSScriptRoot/VrektRotationTest.cpp`" /Fe`"$out/$variant.exe`" /Fo`"$out/$variant.obj`"`nif errorlevel 1 exit /b 1`n`"$out/$variant.exe`" `"$out/update.tsv`"`nexit /b %errorlevel%`n"
 [IO.File]::WriteAllText("$out/run-$variant.cmd",$cmd)
 & "$out/run-$variant.cmd"
 if($LASTEXITCODE){throw "Vrekt native $variant regression failed"}
}
'PASS: repeated candidate restoration, preserved unrelated/operator data, real native lookup/generation and automatic candidate/admission loop versus pinned baseline. Selection, rewards, persistence, DB2 stores and world services are fixture leaves; native typed DB2 catalog is a separate test.'
