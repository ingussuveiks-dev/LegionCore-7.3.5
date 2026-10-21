$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warlock-artifact-choice"
New-Item -ItemType Directory -Force $out | Out-Null
function Block([string]$text,[string]$marker){$a=$text.IndexOf($marker);if($a -lt 0){throw "Missing $marker"};$end=$text.IndexOf('{',$a)+1;$depth=1;while($depth -and $end -lt $text.Length){if($text[$end] -eq '{'){$depth++};if($text[$end] -eq '}'){$depth--};$end++};if($depth){throw "Unbalanced $marker"};$text.Substring($a,$end-$a)}
function Save($name,$value){[IO.File]::WriteAllText("$out/$name.inc",$value)}
Save 'Display' (Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'void Player::SendDisplayPlayerChoice(')
Save 'Handle' (Block (Get-Content "$repo/src/server/game/Handlers/MiscHandler.cpp" -Raw) 'void WorldSession::HandleChoiceResponse(')
Save 'Status' ((Block (Get-Content "$repo/src/server/game/Quests/QuestDef.h" -Raw) 'enum QuestStatus')+';')
$cond=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$cases=@('CONDITION_CLASS','CONDITION_QUESTREWARDED','CONDITION_QUEST_COMPLETE') | ForEach-Object {$a=$cond.IndexOf("        case $_"+':');$b=$cond.IndexOf('        case ',$a+13);$cond.Substring($a,$b-$a)}
Save 'Cases' ($cases -join "`n")
$a=$cond.IndexOf('    if (NegativeCondition)');$b=$cond.IndexOf("`n}",$a);Save 'Tail' $cond.Substring($a,$b-$a)
Save 'List' (Block $cond 'bool ConditionMgr::IsObjectMeetToConditionList(')
$migration=Get-Content "$repo/sql/updates/world/2026_10_21_01_warlock_artifact_choice.sql" -Raw
$sql=@"
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
$migration
$migration
SELECT 'COUNT',COUNT(*) FROM conditions WHERE Comment LIKE 'Warlock artifact choice - %';
SELECT 'OTHER',COUNT(*) FROM saved_conditions a LEFT JOIN conditions b ON a.SourceTypeOrReferenceId=b.SourceTypeOrReferenceId AND a.SourceGroup=b.SourceGroup AND a.SourceEntry=b.SourceEntry AND a.SourceId=b.SourceId AND a.ElseGroup=b.ElseGroup AND a.ConditionTypeOrReference=b.ConditionTypeOrReference AND a.ConditionTarget=b.ConditionTarget AND a.ConditionValue1=b.ConditionValue1 AND a.ConditionValue2=b.ConditionValue2 AND a.ConditionValue3=b.ConditionValue3 AND a.NegativeCondition=b.NegativeCondition WHERE NOT(a.SourceTypeOrReferenceId=15 AND a.SourceGroup=19326 AND a.SourceEntry=0) AND a.Comment NOT LIKE 'Warlock artifact choice - %' AND b.SourceTypeOrReferenceId IS NULL;
SELECT SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE Comment LIKE 'Warlock artifact choice - %' ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionValue1;
"@
$db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$prior=$env:MYSQL_PWD
try {$env:MYSQL_PWD=$db[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Artifact choice temporary SQL fixture failed'}}finally{$env:MYSQL_PWD=$prior}
if($rows[0] -ne "COUNT`t30" -or $rows[1] -ne "OTHER`t0"){throw "Migration scope/repeat failure: $($rows[0]), $($rows[1])"}
[IO.File]::WriteAllLines("$out/conditions.tsv",[string[]]$rows[2..($rows.Count-1)])
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/WarlockArtifactChoiceTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/conditions.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native Warlock artifact choice regression failed'}
