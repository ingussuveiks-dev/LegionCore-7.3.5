param([string]$Migration="$PSScriptRoot/../../sql/updates/world/2026_10_15_30_commanders_valor.sql",[string]$ExitMigration="$PSScriptRoot/../../sql/updates/world/2026_10_15_31_commanders_valor_exit.sql",[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/commanders-valor"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/commanders_valor.cpp" -Raw
$a=$source.IndexOf('namespace CommandersValor');$b=$source.IndexOf('class spell_commanders_valor_location',$a)
[IO.File]::WriteAllText("$out/Valor.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/CommandersValorTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Commander Valor production admission regression failed'}
$mgr=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
$exit=[regex]::Match($mgr,'SMART_EVENT_EVENTOBJECT_OFFTRIGGER\s*=\s*(\d+)')
if(!$exit.Success -or $exit.Groups[1].Value -ne '91' -or $mgr -notmatch '\{SMART_EVENT_EVENTOBJECT_OFFTRIGGER,\s*SMART_SCRIPT_TYPE_MASK_EVENTOBJECT\s*\}') {throw 'Native EO exit event/admission changed'}
$ai=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
if($ai -notmatch 'ProcessEventsFor\(SMART_EVENT_EVENTOBJECT_OFFTRIGGER, player, event->GetEntry\(\)\)'){throw 'Actual EO exit dispatcher missing'}
$migrationSql=(Get-Content $Migration -Raw)+"`n"+(Get-Content $ExitMigration -Raw)
$sql=''
foreach($table in @('conditions','smart_scripts','spell_script_names')){
 $sql+="CREATE TEMPORARY TABLE saved_$table LIKE $table; INSERT INTO saved_$table SELECT * FROM $table; CREATE TEMPORARY TABLE $table LIKE saved_$table; INSERT INTO $table SELECT * FROM saved_$table;`n"
}
$select=@'
SELECT 'gate',SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionValue1,NegativeCondition FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceId=13 AND SourceGroup=1 AND SourceEntry IN(124,134,145,147,148,158,165,167,177,181,198) ORDER BY SourceEntry,ElseGroup,ConditionTypeOrReference;
SELECT 'stale',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceId=10 AND SourceGroup=1 AND SourceEntry IN(124,134,145,147,148,158,165,167,177,181,198);
SELECT 'exit',COUNT(*) FROM smart_scripts WHERE source_type=13 AND id=1 AND event_type=91 AND action_type=28 AND action_param1=219567 AND target_type=7 AND entryorguid IN(124,134,145,147,148,158,165,167,177,181,198);
SELECT 'invalidExit',COUNT(*) FROM smart_scripts WHERE source_type=13 AND id=1 AND event_type=90 AND action_type=28 AND action_param1=219567 AND entryorguid IN(124,134,145,147,148,158,165,167,177,181,198);
SELECT 'bindings',COUNT(*) FROM spell_script_names WHERE (spell_id=219565 AND ScriptName='spell_commanders_valor_location') OR (spell_id=219567 AND ScriptName='aura_commanders_valor_location');
SELECT 'other',COUNT(*) FROM conditions c LEFT JOIN saved_conditions s ON c.SourceTypeOrReferenceId=s.SourceTypeOrReferenceId AND c.SourceGroup=s.SourceGroup AND c.SourceEntry=s.SourceEntry AND c.SourceId=s.SourceId AND c.ElseGroup=s.ElseGroup AND c.ConditionTypeOrReference=s.ConditionTypeOrReference AND c.ConditionValue1=s.ConditionValue1 AND c.ConditionValue2=s.ConditionValue2 AND c.ConditionValue3=s.ConditionValue3 AND c.NegativeCondition=s.NegativeCondition AND c.ConditionTarget=s.ConditionTarget WHERE NOT(c.SourceTypeOrReferenceId=22 AND c.SourceGroup=1 AND c.SourceEntry IN(124,134,145,147,148,158,165,167,177,181,198)) AND s.SourceTypeOrReferenceId IS NULL;
'@
$sql+=$migrationSql+"`n"+$select+"`n"+$migrationSql+"`n"+$select
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try {$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Commander Valor temporary SQL replay failed'}}finally{$env:MYSQL_PWD=$old}
$expectedGates=@(@(124,0,9,43359,0),@(134,0,9,43415,0),@(145,0,9,43412,0),@(147,0,9,43423,0),@(148,0,9,43414,0),@(158,0,9,43425,0),@(165,0,8,43409,1),@(165,0,9,43409,0),@(167,0,9,43424,0),@(177,0,9,43418,0),@(181,0,9,43420,0),@(198,0,9,43407,0),@(198,1,9,43422,0))
foreach($gate in $expectedGates){$line="gate`t"+($gate -join "`t");if(@($rows|Where-Object {$_ -eq $line}).Count -ne 2){throw "Missing or altered native gate: $line"}}
foreach($line in @("stale`t0","exit`t11","invalidExit`t0","bindings`t2","other`t0")){if(@($rows|Where-Object {$_ -eq $line}).Count -ne 2){throw "Commander Valor data mismatch: $line"}}
if($VerifyReleaseBinary){
 $binary=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe"))
 foreach($name in @('spell_commanders_valor_location','aura_commanders_valor_location')){if(!$binary.Contains($name)){throw "Canonical Release lacks $name"}}
}
'PASS: 11 native altar gates, DK OR alternatives, exit cleanup, stale-aura/cast admission; migration twice and unrelated conditions preserved. Client button visibility pending.'
