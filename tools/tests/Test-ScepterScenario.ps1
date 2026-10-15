$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-SmartGameObjectFlags.ps1"
$output="$repo/build-extractors/runtime-test/scepter-scenario"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_14_10_scepter_scenario_sequence.sql" -Raw
$migration+="`n"+(Get-Content "$repo/sql/updates/world/2026_10_15_21_tol_barad_portal_animation.sql" -Raw)
$entries='60,601,602,109838,109955,109956,109964,109981,110021,110030,110323,-370846,251699,251879,11003800'
$fixture=@"
CREATE TEMPORARY TABLE saved_smart LIKE smart_scripts;
INSERT INTO saved_smart SELECT * FROM smart_scripts WHERE entryorguid IN ($entries);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_smart;
INSERT INTO smart_scripts SELECT * FROM saved_smart;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry IN ($entries);
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
CREATE TEMPORARY TABLE saved_go LIKE gameobject_template;
INSERT INTO saved_go SELECT * FROM gameobject_template WHERE entry IN(251879,251699,245924,247351);
CREATE TEMPORARY TABLE gameobject_template LIKE saved_go;
INSERT INTO gameobject_template SELECT * FROM saved_go;
UPDATE gameobject_template SET SpellStateAnimKitID=0 WHERE entry=251699;
CREATE TEMPORARY TABLE saved_creature LIKE creature_template;
INSERT INTO saved_creature SELECT * FROM creature_template WHERE entry IN(109981,110323,109955,109956,110021);
CREATE TEMPORARY TABLE creature_template LIKE saved_creature;
INSERT INTO creature_template SELECT * FROM saved_creature;
UPDATE conditions SET ConditionTarget=0 WHERE SourceEntry=109956 AND SourceGroup=1 AND SourceTypeOrReferenceId=22;
UPDATE smart_scripts SET link=1 WHERE entryorguid IN(601,602) AND source_type=13 AND id=0;
"@
# Export the actual migrated rows after each application; no persistent table
# writes occur. The Python replay consumes these rows rather than a second SQL
# implementation of the intended sequence.
$select=@"
SELECT 'smart',JSON_OBJECT('entry',entryorguid,'source',source_type,'id',id,'link',link,'event',event_type,'phase',event_phase_mask,'flags',event_flags,'e1',event_param1,'e2',event_param2,'e3',event_param3,'e4',event_param4,'action',action_type,'a1',action_param1,'a2',action_param2,'a3',action_param3,'a4',action_param4,'a5',action_param5,'a6',action_param6,'target',target_type,'t1',target_param1,'t2',target_param2,'t3',target_param3,'x',target_x,'y',target_y,'z',target_z) FROM smart_scripts ORDER BY entryorguid,source_type,id;
SELECT 'condition',JSON_OBJECT('entry',SourceEntry,'source',SourceId,'group',SourceGroup,'else',ElseGroup,'type',ConditionTypeOrReference,'target',ConditionTarget,'v1',ConditionValue1,'v2',ConditionValue2,'v3',ConditionValue3,'negative',NegativeCondition) FROM conditions ORDER BY SourceEntry,SourceId,SourceGroup,ElseGroup,ConditionTypeOrReference;
SELECT 'go',JSON_OBJECT('entry',entry,'flags',flags,'ai',AIName,'spell',Data10,'animKit',SpellStateAnimKitID) FROM gameobject_template ORDER BY entry;
SELECT 'creature',JSON_OBJECT('entry',entry,'flags',unit_flags,'faction',faction,'ai',AIName) FROM creature_template ORDER BY entry;
SELECT 'spellclick',JSON_OBJECT('entry',npc_entry,'spell',spell_id,'flags',cast_flags) FROM npc_spellclick_spells WHERE npc_entry IN(109964,110030,110800) ORDER BY npc_entry;
SELECT 'destination',JSON_OBJECT('spell',id,'map',target_map,'x',target_position_x,'y',target_position_y,'z',target_position_z) FROM spell_target_position WHERE id IN(218595,219338,220374) ORDER BY id;
SELECT 'questender',JSON_OBJECT('entry',id,'quest',quest) FROM creature_questender WHERE quest=43153;
"@
$sql=$fixture+"`n"+$migration+"`nSELECT 'PASS1';`n"+$select+"`n"+$migration+"`nSELECT 'PASS2';`n"+$select
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try {
 $env:MYSQL_PWD=$c[3]
 $rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Scepter temporary SQL fixture failed'}
} finally {$env:MYSQL_PWD=$old}
[IO.File]::WriteAllLines("$output/rows.tsv",[string[]]$rows)
& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe -X utf8 "$PSScriptRoot/scepter_scenario_test.py" "$output/rows.tsv"
if($LASTEXITCODE){throw 'Scepter scenario SQL/replay regression failed'}

# Compile the production condition branch and actual SetData method. In
# particular, do not fake a player invoker in this callback regression.
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$a=$source.IndexOf('        case CONDITION_SCENARION_STEP:')
$b=$source.IndexOf('        case CONDITION_REALM_ACHIEVEMENT:', $a)
if($a -lt 0 -or $b -lt $a){throw 'Native scenario condition not found'}
[IO.File]::WriteAllText("$output/ScenarioCondition.inc",$source.Substring($a,$b-$a))
if(!$source.Contains('ConditionSourceInfo sourceInfo(unit, baseObject);')){throw 'Native SmartEvent target binding changed'}
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
$a=$source.IndexOf('void SmartAI::SetData(')
$b=$source.IndexOf("`n}",$a)+2
[IO.File]::WriteAllText("$output/SmartSetData.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$output`" `"$PSScriptRoot/ScepterConditionTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$output/test.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Native null-invoker condition regression failed'}
