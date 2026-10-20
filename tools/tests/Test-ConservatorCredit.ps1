$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/conservator-credit"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Extract([string]$s,[string]$start,[string]$end,[string]$name){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf($end,$a);if($b -lt $a){throw "Missing $end"};[IO.File]::WriteAllText("$out/$name.inc",$s.Substring($a,$b-$a))}
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
Extract $s '        case SMART_TARGET_PLAYER_DISTANCE:' '        case SMART_TARGET_STORED:' 'ConservatorPlayerTargets'
Extract $s 'ObjectList* SmartScript::GetWorldObjectsInDist(' 'void SmartScript::ProcessEvent(' 'ConservatorWorldObjects'
Extract $s '        case SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA:' '        case SMART_ACTION_SUMMON_CONVERSATION:' 'ConservatorCriteria'
$s=Get-Content "$repo/src/server/game/Grids/Notifiers/GridNotifiers.cpp" -Raw
Extract $s 'AllWorldObjectsInRange::AllWorldObjectsInRange(' 'ObjectTypeIdCheck::ObjectTypeIdCheck(' 'ConservatorRange'
$migration=Get-Content "$repo/sql/updates/world/2026_10_20_04_conservator_credit_range.sql" -Raw
$snapshot='SELECT * FROM smart_scripts ORDER BY entryorguid,source_type,id;'
$sql=@"
CREATE TEMPORARY TABLE saved_sai LIKE smart_scripts;
INSERT INTO saved_sai SELECT * FROM smart_scripts WHERE entryorguid IN(101257,100344,100850,10085000);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_sai;
INSERT INTO smart_scripts SELECT * FROM saved_sai;
UPDATE smart_scripts SET target_param1=30 WHERE entryorguid=101257 AND source_type=0 AND id=1;
SELECT 'COLUMNS';
SHOW COLUMNS FROM smart_scripts;
SELECT 'BEFORE';
$snapshot
$migration
SELECT 'FIRST';
$snapshot
$migration
SELECT 'SECOND';
$snapshot
SELECT 'END';
SELECT 'RANGE',target_param1 FROM smart_scripts WHERE entryorguid=101257 AND source_type=0 AND id=1 AND event_type=6 AND action_type=205 AND action_param1=92 AND action_param2=47985 AND target_type=18 AND link=2 AND event_flags=1;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$savedPwd=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Conservator SQL replay failed'}}finally{$env:MYSQL_PWD=$savedPwd}
function Block([string]$first,[string]$last){$a=[Array]::IndexOf($rows,$first);$b=[Array]::IndexOf($rows,$last);if($a -lt 0 -or $b -le $a){throw "Missing $first"};return $rows[($a+1)..($b-1)]}
$columns=@(Block 'COLUMNS' 'BEFORE' | ForEach-Object {($_ -split "`t")[0]});$rangeIndex=[Array]::IndexOf($columns,'target_param1')
if($rangeIndex -lt 0){throw 'Missing native range column'}
$changed=0
$expected=@(Block 'BEFORE' 'FIRST' | ForEach-Object {$fields=$_ -split "`t";if($fields[0] -eq '101257' -and $fields[1] -eq '0' -and $fields[2] -eq '1'){$fields[$rangeIndex]='100';++$changed};$fields -join "`t"})
if($changed -ne 1 -or ($expected -join "`n") -ne ((Block 'FIRST' 'SECOND') -join "`n")){throw 'Unexpected or missing Conservator row changes'}
if(((Block 'FIRST' 'SECOND') -join "`n") -ne ((Block 'SECOND' 'END') -join "`n")){throw 'Migration is not repeatable'}
if($rows -notcontains "RANGE`t100"){throw 'Native death/criteria/link fields changed'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/ConservatorCreditTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" 100`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native Conservator credit regression failed'}
'PASS: only the native death-credit radius changed; linked effects and all other fixture fields/actors preserved; migration repeatable'
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile UlthaleshDialogueCatalog.cpp -ProbeName UlthaleshDialogueCatalog -OutputFile "$out/native.tsv"
$native=Get-Content "$out/native.tsv"
foreach($expected in @('criterion 2 92 47985','criterion 3 92 47987','criterion 4 92 47988','criterion 5 92 47989')){if($native -notcontains $expected){throw "Native scenario route mismatch: $expected"}}
'PASS: native DB2 confirms Conservator -> artifact search -> Ariden approach -> defeat criteria'
