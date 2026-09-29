param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/legion-intro-sequence")
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=[IO.File]::ReadAllText("$repo/src/server/scripts/Scenario/BrokenIslands/broken_islands.cpp")
$start=$source.IndexOf('namespace LegionIntroSequence')
if($start -lt 0){throw 'Production intro repair namespace not found.'}
$open=$source.IndexOf('{',$start); $end=$open+1; $depth=1
while($depth -gt 0 -and $end -lt $source.Length){
 if($source[$end] -eq '{'){++$depth}
 if($source[$end] -eq '}'){--$depth}
 ++$end
}
if($depth -ne 0){throw 'Unbalanced production intro namespace.'}
[IO.File]::WriteAllText("$output/LegionIntroSequence.inc",$source.Substring($start,$end-$start))
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found.'}
$commands=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/LegionIntroSequenceTest.cpp" /Fe"$output/LegionIntroSequenceTest.exe" /Fo"$output/LegionIntroSequenceTest.obj"
if errorlevel 1 exit /b 1
"$output/LegionIntroSequenceTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$commands)
& "$output/run.cmd"
if($LASTEXITCODE -ne 0){throw 'Production intro quest-log repair test failed.'}

# Each modified table is shadowed by a connection-local copy. The migration
# and all progression cases run without touching live world/character rows.
$copies=@{
 quest_template='ID IN (43926,44281,40518,40522,40760,40607,40605,44663)'
 quest_template_addon='ID IN (43926,44281,40518,40522,40760,40607,40605,44663,44120,41002)'
 conditions='(SourceTypeOrReferenceId=19 AND SourceEntry IN (43926,44281,40518,40522,40760,40607,40605,44663)) OR (SourceTypeOrReferenceId=23 AND SourceGroup IN (14,1637,41)) OR (SourceTypeOrReferenceId=15 AND SourceGroup=20487)'
 phase_definitions='zoneId IN (14,1637,41)'
 creature='id IN (101035,95234,100453,100442,100866,100873) AND map=1'
 smart_scripts='entryorguid IN (4311,100873,113986)'
 gossip_menu_option='MenuID IN (20487,19135,20457)'
 spell_scene_event='MiscValue=1449'
}
$sql=foreach($table in $copies.Keys){
 "CREATE TEMPORARY TABLE original_$table AS SELECT * FROM $table WHERE $($copies[$table]);"
 "CREATE TEMPORARY TABLE $table LIKE original_$table;"
 "INSERT INTO $table SELECT * FROM original_$table;"
}
$migration=Get-Content "$repo/sql/updates/world/2026_09_29_332_order_horde_legion_intro.sql" -Raw
$digest="SELECT MD5(GROUP_CONCAT(CONCAT_WS(':',SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment) ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionValue1 SEPARATOR '|')) FROM conditions"
$sql+=@('SET SESSION group_concat_max_len=1000000;',$migration,"SET @first_digest=($digest);",$migration,"SELECT 'idempotent',@first_digest=($digest);")
$sql+='CREATE TEMPORARY TABLE intro_cases (caseId INT PRIMARY KEY,label VARCHAR(100),team INT,classMask INT,expectedQuest INT,zoneId INT,expectedPhase VARCHAR(40));'
$sql+='CREATE TEMPORARY TABLE intro_states (caseId INT,quest INT,status INT,allari INT,funeral INT,PRIMARY KEY(caseId,quest));'
$ordered=@(43926,44281,40518,40522,40760,40607,40605,44663)
$script:caseId=0
function Add-Case([string]$label,[hashtable]$states,[int]$expectedQuest,[string]$expectedPhase,[int]$zone=14,[int]$allari=0,[int]$funeral=0,[int]$team=67,[int]$classMask=256){
 ++$script:caseId
 $script:sql+="INSERT INTO intro_cases VALUES ($script:caseId,'$label',$team,$classMask,$expectedQuest,$zone,'$expectedPhase');"
 foreach($quest in $states.Keys){$script:sql+="INSERT INTO intro_states VALUES ($script:caseId,$quest,$($states[$quest]),$(if($quest -eq 40607){$allari}else{0}),$(if($quest -eq 40522){$funeral}else{0}));"}
}
$states=@{}
for($i=0;$i -lt $ordered.Count;++$i){
 $phase=if($i -eq 0){'0'}elseif($i -le 2){'2'}elseif($i -eq 3){'4'}elseif($i -le 5){'3'}elseif($i -eq 6){'150'}else{'0'}
 Add-Case "next_$($ordered[$i])" $states $ordered[$i] $phase
 $current=$states.Clone(); $current[$ordered[$i]]=3
 $acceptedPhase=if($i -eq 0){'2'}else{$phase}
 Add-Case "accepted_$($ordered[$i])" $current 0 $acceptedPhase
 $current[$ordered[$i]]=1
 $completedPhase=if($i -eq 2){'4'}elseif($i -eq 3){'3'}elseif($i -eq 5){'1'}else{$acceptedPhase}
 Add-Case "complete_not_turned_in_$($ordered[$i])" $current 0 $completedPhase -allari $(if($i -eq 5){1}else{0}) -funeral $(if($i -eq 3){1}else{0})
 $states[$ordered[$i]]=6
}
$afterFate=@{43926=6;44281=6;40518=6;40522=6;40760=6;40607=3}
Add-Case 'Allari_revealed_demons' $afterFate 0 '1' -allari 1
$afterDemons=@{40518=6;40522=6;40760=6;40607=6}
Add-Case 'boost_route_after_demons' $afterDemons 40605 '150'
$legacy=$afterDemons.Clone(); $legacy[44663]=1
Add-Case 'legacy_skip_before_login_repair' $legacy 0 '150'
$afterIllidari=$afterDemons.Clone(); $afterIllidari[40605]=6; $afterIllidari[44663]=3
Add-Case 'Dalaran_portal_incomplete' $afterIllidari 0 '8' -zone 1637
$afterIllidari[44663]=1
Add-Case 'Dalaran_portal_complete' $afterIllidari 0 '8' -zone 1637
$afterIllidari[44663]=6
Add-Case 'Dalaran_portal_rewarded' $afterIllidari 0 '7' -zone 1637
Add-Case 'Alliance_shared_Dalaran_handoff' @{44120=6} 44663 '0' -team 469
Add-Case 'DH_shared_Dalaran_handoff' @{41002=6} 44663 '0' -classMask 2048
Add-Case 'Dalaran_requires_turned_in_Illidari_handoff' @{40518=6;40522=6;40760=6;40607=6;40605=1} 0 '150'
Add-Case 'funeral_in_Orgrimmar' @{43926=6;44281=6;40518=6;40522=3} 0 '6,54' -zone 1637
Add-Case 'funeral_finished_in_Orgrimmar' @{43926=6;44281=6;40518=6;40522=3} 0 '0' -zone 1637 -funeral 1
Add-Case 'old_Dalaran_council_before_scene' @{40518=6;40522=6;40760=6;40607=6;40605=6;44663=3} 0 '1000' -zone 41
Add-Case 'old_Dalaran_phase_ends_after_transfer' @{40518=6;40522=6;40760=6;40607=6;40605=6;44663=1} 0 '0' -zone 41
Add-Case 'old_Dalaran_phase_ends_after_reward' @{40518=6;40522=6;40760=6;40607=6;40605=6;44663=6} 0 '0' -zone 41
$sql+=@'
CREATE TEMPORARY TABLE intro_eval AS
SELECT t.caseId,c.SourceTypeOrReferenceId AS source,c.SourceGroup AS zone,c.SourceEntry AS entry,c.ElseGroup,
MIN((CASE c.ConditionTypeOrReference
 WHEN 6 THEN t.team=c.ConditionValue1
 WHEN 15 THEN (t.classMask & c.ConditionValue1)<>0
 WHEN 8 THEN COALESCE(s.status,0)=6
 WHEN 9 THEN COALESCE(s.status,0)=3
 WHEN 14 THEN COALESCE(s.status,0)=0
 WHEN 28 THEN COALESCE(s.status,0)=1
 WHEN 41 THEN (CASE WHEN c.ConditionValue2=112731 THEN COALESCE(s.allari,0) WHEN c.ConditionValue2=100934 THEN COALESCE(s.funeral,0) ELSE 0 END)>=IF(c.ConditionValue3=0,1,c.ConditionValue3)
 ELSE 0 END)<>c.NegativeCondition) AS allowed
FROM intro_cases t CROSS JOIN conditions c LEFT JOIN intro_states s ON s.caseId=t.caseId AND s.quest=c.ConditionValue1
WHERE c.SourceTypeOrReferenceId IN (19,23)
GROUP BY t.caseId,c.SourceTypeOrReferenceId,c.SourceGroup,c.SourceEntry,c.ElseGroup;
SELECT 'quest',t.label,t.expectedQuest,COALESCE(GROUP_CONCAT(DISTINCT IF(e.allowed=1 AND COALESCE(s.status,0)=0,e.entry,NULL) ORDER BY e.entry),'0')
FROM intro_cases t LEFT JOIN intro_eval e ON e.caseId=t.caseId AND e.source=19
LEFT JOIN intro_states s ON s.caseId=t.caseId AND s.quest=e.entry
GROUP BY t.caseId,t.label,t.expectedQuest ORDER BY t.caseId;
SELECT 'phase',t.label,t.expectedPhase,COALESCE(GROUP_CONCAT(DISTINCT IF(e.allowed=1,e.entry,NULL) ORDER BY e.entry),'0')
FROM intro_cases t LEFT JOIN intro_eval e ON e.caseId=t.caseId AND e.source=23 AND e.zone=t.zoneId
AND ((e.zone=14 AND e.entry IN (1,2,3,4,150)) OR (e.zone=1637 AND e.entry IN (6,7,8,54)) OR (e.zone=41 AND e.entry=1000))
GROUP BY t.caseId,t.label,t.expectedPhase ORDER BY t.caseId;
SELECT 'handoff_npc_mask',phaseMask FROM creature WHERE id=101035 AND PhaseId='6745';
SELECT 'handoff_phase_mask',phasemask FROM phase_definitions WHERE zoneId=14 AND entry=150;
SELECT 'post_Dalaran_normal_city_visible',(phasemask & 1) FROM phase_definitions WHERE zoneId=1637 AND entry=7;
SELECT 'Holgar_skip_rows',COUNT(*) FROM smart_scripts WHERE entryorguid=4311;
SELECT 'Khadgar_link',link FROM smart_scripts WHERE entryorguid=113986 AND source_type=0 AND id=0;
SELECT 'Allari_actions_preserved',COUNT(*) FROM smart_scripts WHERE entryorguid=100873;
SELECT 'shared_Dalaran_previous_quest',PrevQuestID FROM quest_template_addon WHERE ID=44663;
SELECT 'Dalaran_scene_teleport',trigerSpell FROM spell_scene_event WHERE MiscValue=1449 AND Event='complete';
SELECT 'Dalaran_scene_credit',MonsterCredit FROM spell_scene_event WHERE MiscValue=1449 AND Event='complete';
SELECT 'Dalaran_scene_package',SceneScriptPackageID FROM spell_scene WHERE MiscValue=1449;
SELECT 'Dalaran_first_portal_map',target_map FROM spell_target_position WHERE id=228327;
SELECT 'Dalaran_final_transfer_map',target_map FROM spell_target_position WHERE id=230156;
'@
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object{$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$mysql=Get-ChildItem C:/wamp64/bin/mysql -Recurse -Filter mysql.exe | Select-Object -First 1 -ExpandProperty FullName
$previousPassword=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 $rows=($sql -join [Environment]::NewLine) | & $mysql --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names
 if($LASTEXITCODE -ne 0){throw 'Isolated intro migration fixture failed.'}
}finally{$env:MYSQL_PWD=$previousPassword}
$rows | Set-Content "$output/fixture-results.tsv" -Encoding UTF8
$checks=$rows | Where-Object{$_ -match '^(quest|phase)\t'}
if($checks.Count -ne $script:caseId*2){throw 'Missing progression checks.'}
foreach($row in $checks){$f=$row.Split([char]9);if($f[2] -ne $f[3]){throw "Intro sequence case failed: $row"}}
$expected=@{idempotent=1;handoff_npc_mask=1;handoff_phase_mask=1;post_Dalaran_normal_city_visible=1;Holgar_skip_rows=0;Khadgar_link=0;Allari_actions_preserved=2;shared_Dalaran_previous_quest=44120;Dalaran_scene_teleport=230156;Dalaran_scene_credit=114506;Dalaran_scene_package=1728;Dalaran_first_portal_map=0;Dalaran_final_transfer_map=1220}
foreach($key in $expected.Keys){if($rows -notcontains ($key+[char]9+$expected[$key])){throw "Migration integrity check failed: $key"}}
"PASS: $($checks.Count) quest/phase progression checks, deterministic repeated migration, NPC/portal masks, shared routes and existing Allari actions."
