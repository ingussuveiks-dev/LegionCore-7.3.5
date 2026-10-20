$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/ulthalesh-dialogue"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Extract([string]$s,[string]$start,[string]$end,[string]$name){
 $a=$s.IndexOf($start);if($a -lt 0){throw "Missing production section $start"};$b=$s.IndexOf($end,$a)
 if($b -lt $a){throw "Missing section end $end"};[IO.File]::WriteAllText("$out/$name.inc",$s.Substring($a,$b-$a))
}
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
Extract $s '        case SMART_ACTION_TALK:' '        case SMART_ACTION_SIMPLE_TALK:' 'DialogueTalk'
Extract $s '        case SMART_TARGET_SELF:' '        case SMART_TARGET_HOSTILE_SECOND_AGGRO:' 'DialogueTargets'
Extract $s '        case SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA:' '        case SMART_ACTION_SUMMON_CONVERSATION:' 'DialogueCriteria'
Extract $s '    if (e.link && e.link != e.event_id)' 'void SmartScript::InstallTemplate(' 'DialogueLink'
Extract $s 'void SmartScript::ProcessEventsFor(' 'void SmartScript::ProcessAction(' 'DialogueEvents'
$event=$s.Substring($s.IndexOf('void SmartScript::ProcessEvent('))
Extract $event '    if (!e.active && e.GetEventType() != SMART_EVENT_LINK)' '    switch (e.GetEventType())' 'DialogueEventAdmission'
Extract $event '        case SMART_EVENT_HEALTH_PCT:' '        case SMART_EVENT_TARGET_HEALTH_PCT:' 'DialogueHealth'
Extract $event '        case SMART_EVENT_TEXT_OVER:' '        case SMART_EVENT_DATA_SET:' 'DialogueTextOver'
$action=$s.Substring($s.IndexOf('void SmartScript::ProcessAction('))
Extract $action '    if (GetBaseObject() && GetBaseObject()->IsQuestObjectExpired())' '    switch (e.GetActionType())' 'DialogueActionAdmission'
$timer=$s.Substring($s.IndexOf('    if (mUseTextTimer && me)'));$timer=$timer.Substring(0,$timer.IndexOf('void SmartScript::FillScript('));$timer=$timer.Substring(0,$timer.LastIndexOf('}'))
[IO.File]::WriteAllText("$out/DialogueTimer.inc",$timer)
$migration=Get-Content "$repo/sql/updates/world/2026_10_20_03_ulthalesh_dialogue_speaker.sql" -Raw
$native="SELECT id,link,event_type,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,target_type,target_param1 FROM smart_scripts WHERE entryorguid=100344 AND source_type=0 ORDER BY id;"
$snapshot='SELECT * FROM smart_scripts ORDER BY entryorguid,source_type,id;'
$sql=@"
CREATE TEMPORARY TABLE saved_sai LIKE smart_scripts;
INSERT INTO saved_sai SELECT * FROM smart_scripts WHERE entryorguid IN(100344,101282,100388,100735);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_sai;
INSERT INTO smart_scripts SELECT * FROM saved_sai;
UPDATE smart_scripts SET target_type=2 WHERE entryorguid=100344 AND source_type=0 AND id IN(1,4,11,90) AND action_type=1;
SELECT 'COLUMNS';
SHOW COLUMNS FROM smart_scripts;
SELECT 'BEFORE';
$snapshot
SELECT 'BASELINE_NATIVE';
$native
$migration
SELECT 'FIRST';
$snapshot
SELECT 'PATCHED_NATIVE';
$native
$migration
SELECT 'SECOND';
$snapshot
SELECT 'END';
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$savedPwd=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Ulthalesh dialogue SQL replay failed'}}finally{$env:MYSQL_PWD=$savedPwd}
function Block([string]$first,[string]$last){$a=[Array]::IndexOf($rows,$first);$b=[Array]::IndexOf($rows,$last);if($a -lt 0 -or $b -le $a){throw "Missing fixture block $first"};return $rows[($a+1)..($b-1)]}
$columns=@(Block 'COLUMNS' 'BEFORE' | ForEach-Object {($_ -split "`t")[0]})
$targetIndex=[Array]::IndexOf($columns,'target_type');$idIndex=[Array]::IndexOf($columns,'id')
if($targetIndex -lt 0 -or $idIndex -lt 0){throw 'Missing native SmartAI columns'}
$changed=0
$expected=@(Block 'BEFORE' 'BASELINE_NATIVE' | ForEach-Object {
 $fields=$_ -split "`t"
 if($fields[0] -eq '100344' -and $fields[1] -eq '0' -and [int]$fields[$idIndex] -in @(1,4,11,90)){
  if($fields[$targetIndex] -ne '2'){throw 'Fixture lacks the observed VICTIM target'};$fields[$targetIndex]='1';++$changed
 }
 $fields -join "`t"
})
if($changed -ne 4){throw "Expected four scoped speaker repairs, got $changed"}
if(($expected -join "`n") -ne ((Block 'FIRST' 'PATCHED_NATIVE') -join "`n")){throw 'Migration changed unexpected fields/actors or missed a speaker'}
if(((Block 'FIRST' 'PATCHED_NATIVE') -join "`n") -ne ((Block 'SECOND' 'END') -join "`n")){throw 'Dialogue migration is not repeatable'}
[IO.File]::WriteAllLines("$out/baseline.tsv",[string[]](Block 'BASELINE_NATIVE' 'FIRST'))
[IO.File]::WriteAllLines("$out/patched.tsv",[string[]](Block 'PATCHED_NATIVE' 'SECOND'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/UlthaleshDialogueTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/baseline.tsv`" baseline`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/patched.tsv`" patched`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Ulthalesh native dialogue regression failed'}
'PASS: four speaker fields changed; all other fixture fields/actors preserved; repeated migration stable'
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile UlthaleshDialogueCatalog.cpp -ProbeName UlthaleshDialogueCatalog -OutputFile "$out/native.tsv"
$native=Get-Content "$out/native.tsv"
foreach($expected in @('criterion 4 92 47988','criterion 5 92 47989','criterion 6 36 128942')){if($native -notcontains $expected){throw "Native Ulthalesh scenario criterion mismatch: $expected"}}
'PASS: native runtime ScenarioStep/CriteriaTree/Criteria confirm aggro, defeat and OWN_ITEM order'
