param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/xalatath-pickup-link"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_24_35_xalatath_pickup_link.sql" -Raw
$reset=@'
DELETE FROM smart_scripts; INSERT INTO smart_scripts SELECT * FROM saved_smart;
UPDATE smart_scripts SET event_type=70,event_param1=3 WHERE id=1;
DELETE FROM gameobject_template; INSERT INTO gameobject_template SELECT * FROM saved_object;
DELETE FROM conditions; INSERT INTO conditions SELECT * FROM saved_conditions;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=247690 AND SourceId=1;
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
VALUES(22,3,247690,1,0,2,0,128827,1,0,0,0,'','Smart Only If Player Has Artifact');
'@
$sql=@"
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid=247690 AND source_type=1;
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
CREATE TEMPORARY TABLE saved_object LIKE gameobject_template;
INSERT INTO saved_object SELECT * FROM gameobject_template WHERE entry=247690;
CREATE TEMPORARY TABLE gameobject_template LIKE saved_object;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceEntry=247690;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
$reset
SELECT 'BEFORE',smart_scripts.* FROM smart_scripts ORDER BY id;
SELECT 'CONDBEFORE',conditions.* FROM conditions;
SELECT 'OBJECTBEFORE',gameobject_template.* FROM gameobject_template;
$migration
SELECT 'FIRST',smart_scripts.* FROM smart_scripts ORDER BY id;
$migration
SELECT 'SECOND',smart_scripts.* FROM smart_scripts ORDER BY id;
SELECT 'CONDAFTER',conditions.* FROM conditions;
SELECT 'OBJECTAFTER',gameobject_template.* FROM gameobject_template;
SELECT 'event',id,link,event_type,event_flags,event_param1,action_type,action_param1,action_param2 FROM smart_scripts ORDER BY id;
"@
$variants=[ordered]@{
 'custom-ai'="UPDATE gameobject_template SET AIName='';"
 'custom-script'="UPDATE gameobject_template SET ScriptName='custom_blade';"
 'custom-object-type'='UPDATE gameobject_template SET type=10;'
 'custom-root'='UPDATE smart_scripts SET action_param1=1 WHERE id=0;'
 'custom-gossip'='UPDATE smart_scripts SET event_flags=0 WHERE id=2;'
 'custom-criterion'='UPDATE smart_scripts SET action_param2=1 WHERE id=1;'
 'custom-link'='UPDATE smart_scripts SET link=0 WHERE id=1;'
 'custom-target'='UPDATE smart_scripts SET target_type=21 WHERE id=1;'
 'custom-phase'='UPDATE smart_scripts SET event_phase_mask=1 WHERE id=1;'
 'custom-later-spell'='UPDATE smart_scripts SET action_param1=1 WHERE id=3;'
 'missing-later-row'='DELETE FROM smart_scripts WHERE id=4;'
 'missing-ownership'='DELETE FROM conditions;'
 'custom-ownership'='UPDATE conditions SET ConditionValue2=2;'
 'extra-guard'='INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ConditionTypeOrReference,ConditionValue1) VALUES(22,2,247690,1,9,1);'
}
foreach($name in $variants.Keys){$sql+="$reset`n$($variants[$name])`nSELECT '$name-before',smart_scripts.* FROM smart_scripts ORDER BY id;`n$migration`nSELECT '$name-after',smart_scripts.* FROM smart_scripts ORDER BY id;`n"}
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
function Query([string]$query){$previous=$env:MYSQL_PWD;try{$env:MYSQL_PWD=$connection[3];$result=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Xalatath TEMP SQL failed'};return $result}finally{$env:MYSQL_PWD=$previous}}
$rows=Query $sql
function Snapshot([string]$label){@($rows|Where-Object {$_ -like "$label`t*"}|ForEach-Object {$_.Substring($label.Length+1)})}
$first=Snapshot 'FIRST';if($first.Count -ne 5 -or (Compare-Object $first (Snapshot 'SECOND'))){throw 'Xalatath link migration not idempotent'}
$columns=@(Query 'SHOW COLUMNS FROM smart_scripts;'|ForEach-Object {$_.Split("`t")[0]})
$expected=@(Snapshot 'BEFORE'|ForEach-Object {$fields=$_.Split("`t");if($fields[$columns.IndexOf('id')] -eq '1'){$fields[$columns.IndexOf('event_type')]='61';$fields[$columns.IndexOf('event_param1')]='0'};$fields -join "`t"})
if(Compare-Object $first $expected){throw 'Changed fields beyond linked event type/state parameter'}
foreach($prefix in @('COND','OBJECT')){if(Compare-Object (Snapshot "${prefix}BEFORE") (Snapshot "${prefix}AFTER")){throw "Changed $prefix"}}
foreach($name in $variants.Keys){if(Compare-Object (Snapshot "$name-before") (Snapshot "$name-after")){throw "Custom pickup overwritten: $name"}}
$events=@(Snapshot 'event'|ForEach-Object {$fields=$_.Split("`t");if($Baseline -and $fields[0] -eq '1'){$fields[2]='70';$fields[4]='3'};$fields -join "`t"})
[IO.File]::WriteAllLines("$out/events.tsv",[string[]]$events)
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first) -replace '\be\b','nativeEvent'}
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
[IO.File]::WriteAllText("$out/Find.inc",(Section $source 'SmartScriptHolder SmartScript::FindLinkedEvent(' 'void SmartScript::OnUpdate('))
[IO.File]::WriteAllText("$out/Repeat.inc",(Section $source '    if ((e.event.event_phase_mask && !IsInPhase(' '    ConditionList conds ='))
[IO.File]::WriteAllText("$out/State.inc",(Section $source '        case SMART_EVENT_GO_STATE_CHANGED:' '        case SMART_EVENT_GO_EVENT_INFORM:'))
$link=Section $source '    if (e.link && e.link != e.event_id)' 'void SmartScript::InstallTemplate('
[IO.File]::WriteAllText("$out/Link.inc",$link.Substring(0,$link.LastIndexOf('}')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/XalatathPickupLinkTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/events.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Xalatath LINK replay failed'}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile XalatathPickupCatalog.cpp -ProbeName XalatathPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv";$native
foreach($expected in @("EFFECT`t201780`t0`t6`t430`t1187`t0","EFFECT`t204919`t0`t6`t293`t667`t0","EFFECT`t201917`t0`t219`t0`t1146`t0")){if($native -notcontains $expected){throw "Native pickup payload changed: $expected"}}
if($VerifyReleaseBinary){
 $hash=(Get-FileHash "$repo/sql/updates/world/2026_10_24_35_xalatath_pickup_link.sql" -Algorithm SHA1).Hash
 $live=Query "SELECT 'release',hash,state FROM updates WHERE name='2026_10_24_35_xalatath_pickup_link.sql'; SELECT 'link',event_type,event_param1,action_param2 FROM smart_scripts WHERE entryorguid=247690 AND source_type=1 AND id=1;"
 foreach($expected in @("release`t$hash`tRELEASED","link`t61`t0`t48493")){if($live -notcontains $expected){throw "Live pickup link mismatch: $expected"}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_xalatath_pickup_link_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical GO/LINK/criterion dispatcher missing'}}finally{$env:PYTHONPATH=$previous}
}
"PASS: only invalid pickup event1/state3 becomes LINK61/state0;14 custom variants and original ownership/object/spell/credit fields retained. Loot/equip/client traversal pending."
