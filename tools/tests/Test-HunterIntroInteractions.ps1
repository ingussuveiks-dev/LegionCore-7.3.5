$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hunter-intro-interactions"
New-Item -ItemType Directory -Force $out | Out-Null
function Extract($source,$start,$end,$file){$a=$source.IndexOf($start);$b=$source.IndexOf($end,$a);if($a -lt 0 -or $b -lt $a){throw "Missing native section $start"};[IO.File]::WriteAllText("$out/$file.inc",$source.Substring($a,$b-$a))}
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
Extract $s '        case SMART_ACTION_CLOSE_GOSSIP:' '        case SMART_ACTION_EQUIP:' HunterCloseGossip
Extract $s '    if (e.link && e.link != e.event_id)' 'void SmartScript::InstallTemplate(' HunterLinkTail
$s=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
Extract $s '        case CONDITION_QUESTTAKEN:' '        case CONDITION_QUEST_NONE:' HunterQuestConditions
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/HunterIntroInteractionsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native Hunter interaction regression failed'}
$migration=Get-Content "$repo/sql/updates/world/2026_10_23_01_hunter_intro_interactions.sql" -Raw
$snapshot=@'
SELECT 'link',entryorguid,id,event_type,action_type,target_type FROM smart_scripts WHERE source_type=0 AND entryorguid IN (100810,99986,100171) AND id=1 ORDER BY entryorguid;
SELECT 'click',ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1 FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=106854 AND SourceEntry=212060 ORDER BY ElseGroup;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_sai LIKE smart_scripts;
INSERT INTO saved_sai SELECT * FROM smart_scripts WHERE entryorguid IN (100810,99986,100171,106854,104904,104949);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_sai;
INSERT INTO smart_scripts SELECT * FROM saved_sai;
DELETE FROM smart_scripts WHERE entryorguid IN (100810,99986,100171) AND source_type=0 AND id=1;
CREATE TEMPORARY TABLE saved_cond LIKE conditions;
INSERT INTO saved_cond SELECT * FROM conditions WHERE SourceEntry IN (106854,212060);
CREATE TEMPORARY TABLE conditions LIKE saved_cond;
INSERT INTO conditions SELECT * FROM saved_cond;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=106854 AND SourceEntry=212060;
$migration
SELECT 'FIRST';
$snapshot
$migration
SELECT 'SECOND';
$snapshot
SELECT 'END';
SELECT 'return',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=106854 AND SourceGroup=3 AND ConditionTypeOrReference=44 AND ConditionValue1=1099 AND ConditionValue2=5;
SELECT 'saved-return',COUNT(*) FROM saved_cond WHERE SourceTypeOrReferenceId=22 AND SourceEntry=106854 AND SourceGroup=3 AND ConditionTypeOrReference=44 AND ConditionValue1=1099 AND ConditionValue2=5;
UPDATE smart_scripts SET action_type=1 WHERE source_type=0 AND entryorguid=100810 AND id=1;
UPDATE conditions SET ConditionValue1=12345 WHERE SourceTypeOrReferenceId=18 AND SourceGroup=106854 AND SourceEntry=212060;
$migration
SELECT 'custom-link',COUNT(*) FROM smart_scripts WHERE entryorguid=100810 AND source_type=0 AND id=1 AND action_type=1;
SELECT 'custom-click',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=106854 AND SourceEntry=212060 AND ConditionValue1=12345;
SELECT 'timing',SUM(event_param1) FROM smart_scripts WHERE entryorguid=104904 AND source_type=9;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Hunter interaction SQL fixture failed'}}finally{$env:MYSQL_PWD=$previous}
$a=[Array]::IndexOf($rows,'FIRST');$b=[Array]::IndexOf($rows,'SECOND');$d=[Array]::IndexOf($rows,'END')
$first=$rows[($a+1)..($b-1)];$second=$rows[($b+1)..($d-1)]
if(($first -join '|') -ne ($second -join '|')){throw 'Hunter interaction migration is not idempotent'}
$expected=@("link`t99986`t1`t61`t72`t7","link`t100171`t1`t61`t72`t7","link`t100810`t1`t61`t72`t7","click`t0`t9`t0`t41574","click`t1`t28`t0`t41574")
if(($first -join '|') -ne ($expected -join '|')){throw "Wrong migrated interactions: $($first -join '|')"}
foreach($r in @("custom-link`t1","custom-click`t2","timing`t55000")){if($rows -notcontains $r){throw "Missing preservation result $r"}}
$return=($rows|Where-Object {$_ -like "return`t*"}).Split("`t")[1]
$savedReturn=($rows|Where-Object {$_ -like "saved-return`t*"}).Split("`t")[1]
if([int]$return -lt 1 -or $return -ne $savedReturn){throw 'Existing final-return scenario conditions changed'}
$player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$player.IndexOf('void Player::UpdateForQuestWorldObjects(');$section=$player.Substring($a,6000)
if(!$section.Contains('CONDITION_QUESTTAKEN') -or !$section.Contains('GetConditionsForSpellClickEvent')){throw 'Quest change must dirty conditioned spellclick NPC flags'}
'PASS: actual SQL twice, three complete gossip links, Huey active/complete click gate, native 1099 return and 55s introduction retained; custom definitions preserved. Client click/flag transitions still require verification.'
