param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-return-credit"
New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Test-WarswordsPickupAdmission.ps1" -ReturnBaseline *> "$out/baseline.log"
if($LASTEXITCODE){throw 'Pinned premature return-credit baseline failed'}
& "$PSScriptRoot/Test-WarswordsPickupAdmission.ps1" *> "$out/pickup.log"
if($LASTEXITCODE){throw 'Fixed return-credit callback failed'}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 $rows=@(@'
SELECT 'quest',ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID=40043 ORDER BY ID;
SELECT 'actor',entry,AIName,ScriptName FROM creature_template WHERE entry=93823;
SELECT 'spawn',guid,id,map,zoneId,position_x,position_y,position_z,PhaseId FROM creature WHERE guid=369004;
SELECT 'credit',entryorguid,source_type,id,link,event_type,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,target_type,target_param1 FROM smart_scripts WHERE entryorguid=93823 AND source_type=0 AND id=10;
SELECT 'actor-conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=93823 AND SourceGroup=11 AND SourceId=0;
SELECT 'phase',zoneId,entry,phasemask,phaseId,flags FROM phase_definitions WHERE zoneId=7813 AND entry=1;
SELECT 'phase-conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7813 AND SourceEntry=1;
SELECT 'target',id,target_map,target_position_x,target_position_y,target_position_z FROM spell_target_position WHERE id=216016;
'@|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Existing native Skyhold receiver read failed'}
}finally{$env:MYSQL_PWD=$previous}
[IO.File]::WriteAllLines("$out/live.tsv",[string[]]$rows)
foreach($expected in @("quest`t280193`t40043`t0`t0`t98032`t1`t0","quest`t280194`t40043`t0`t1`t98033`t1`t2","quest`t282124`t40043`t0`t2`t103739`t1`t2","actor`t93823`tSmartAI`t","spawn`t369004`t93823`t1479`t7813`t1059.61`t7224.6`t100.378`t5997","credit`t93823`t0`t10`t0`t60`t100`t0`t0`t0`t2000`t2000`t33`t103739`t18`t50","actor-conditions`t0","phase`t7813`t1`t0`t5997`t16","phase-conditions`t0","target`t216016`t1479`t1028.64`t7225.23`t100.18")){
 if($rows -notcontains $expected){throw "Existing return route differs: $expected"}
}
$distance=[math]::Sqrt([math]::Pow(1059.61-1028.64,2)+[math]::Pow(7224.6-7225.23,2)+[math]::Pow(100.378-100.18,2))
if($distance -ge 50){throw 'Native return target outside existing receiver radius'}
& "$PSScriptRoot/Test-ScriptPetCredit.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary *> "$out/native-dispatch.log"
if($LASTEXITCODE){throw 'Native SmartAI action33/player-owner dispatch failed'}
& "$PSScriptRoot/Test-SkyholdJumpDestination.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary *> "$out/native-jump.log"
if($LASTEXITCODE){throw 'Native jump/delayed destination regression failed'}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_warswords_return_credit_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical premature-credit repair missing'}}finally{$env:PYTHONPATH=$previous}
}
"PASS: failed/accepted jump requests do not grant return credit; existing Skyhold actor/base phase/2s action33/native dispatch and destination within $([math]::Round($distance,2))yd checked. Actual phase visibility, arrival, group/quest counters and client button/travel remain separate."
