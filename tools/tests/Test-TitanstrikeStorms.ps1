param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/titanstrike-storms"
New-Item -ItemType Directory -Force $out | Out-Null
$source=Get-Content "$repo/src/server/scripts/Legion/titanstrike_storms.cpp" -Raw
$a=$source.IndexOf('struct at_titanstrike_spirit_winds');$b=$source.IndexOf('void AddSC_titanstrike_storms()',$a)
[IO.File]::WriteAllText("$out/TitanstrikeStormArea.inc",$source.Substring($a,$b-$a))
$nativeCreate=Get-Content "$repo/src/server/game/Entities/AreaTrigger/AreaTrigger.cpp" -Raw
$visualSelection=$nativeCreate.IndexOf('SetUInt32Value(AREATRIGGER_FIELD_SPELL_XSPELL_VISUAL_ID, info->GetSpellXSpellVisualId(caster));')
$callback=$nativeCreate.IndexOf('_ai->CalculateDuration(_duration);',$visualSelection)
$publish=$nativeCreate.IndexOf('if (!GetMap()->AddToMap(this))',$callback)
if($visualSelection -lt 0 -or $callback -le $visualSelection -or $publish -le $callback){throw 'Native pre-publish visual callback order changed'}
$source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/shields_rest.cpp" -Raw
$actors=''
foreach($name in @('npc_sr_spectral_windshaperAI','npc_sr_fierce_windsAI')){
 $a=$source.IndexOf("struct $name");$b=$source.IndexOf('CreatureAI* GetAI(', $a)
 if($a -lt 0 -or $b -lt $a){throw 'Production storm actor missing'}
 $actors+=$source.Substring($a,$b-$a)
}
[IO.File]::WriteAllText("$out/TitanstrikeStormActors.inc",$actors)
$source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/instance_shields_rest.cpp" -Raw
$a=$source.IndexOf('uint32 hunterWindTimer');$b=$source.IndexOf("`n    };",$a)
[IO.File]::WriteAllText("$out/TitanstrikeStormFloor.inc",$source.Substring($a,$b-$a))
$source=Get-Content "$repo/src/server/scripts/Scenario/UngaIngoo/unga_ingoo.cpp" -Raw
$a=$source.IndexOf('class npc_unga_banana_ship_script');$b=$source.IndexOf('// Unga Pirates',$a)
[IO.File]::WriteAllText("$out/TitanstrikeInvisibleFactory.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/TitanstrikeStormsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Production storm regression failed'}
$migration=Get-Content "$repo/sql/updates/world/2026_10_23_04_titanstrike_spirit_winds.sql" -Raw
$sql=@"
CREATE TEMPORARY TABLE saved_scripts LIKE areatrigger_scripts;
CREATE TEMPORARY TABLE saved_actions LIKE areatrigger_actions;
CREATE TEMPORARY TABLE areatrigger_scripts LIKE saved_scripts;
CREATE TEMPORARY TABLE areatrigger_actions LIKE saved_actions;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=64367 AND SourceGroup=1;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
UPDATE conditions SET ConditionTarget=0 WHERE ConditionTypeOrReference=29;
$migration
UPDATE areatrigger_actions SET spellId=0 WHERE entry=6837 AND customEntry=11360 AND id=0;
$migration
SELECT 'tracking',COUNT(*) FROM areatrigger_actions WHERE entry=6837 AND customEntry=11360 AND moment=1 AND actionType=15 AND targetFlags=8 AND spellId=210880;
SELECT 'binding',COUNT(*) FROM areatrigger_scripts WHERE entry=11360 AND ScriptName='at_titanstrike_spirit_winds';
SELECT 'base-actor',COUNT(*) FROM conditions WHERE ConditionTypeOrReference=29 AND ConditionTarget=1;
UPDATE areatrigger_scripts SET ScriptName='custom' WHERE entry=11360;
DELETE FROM areatrigger_actions;
$migration
SELECT 'custom',COUNT(*) FROM areatrigger_scripts WHERE ScriptName='custom';
SELECT 'no-custom-action',COUNT(*) FROM areatrigger_actions;
"@
$config=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$config[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($config[0]) --port=$($config[1]) --user=$($config[2]) --database=$($config[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Storm SQL fixture failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("tracking`t1","binding`t1","base-actor`t1","custom`t1","no-custom-action`t0")){if($rows -notcontains $expected){throw "Missing $expected"}}
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile TitanstrikeStormCatalog.cpp -ProbeName TitanstrikeStormCatalog -OutputFile "$out/native.tsv"
$native=Get-Content "$out/native.tsv"
foreach($id in @(210976,210977,210978)){if($native -notcontains "EFFECT`t$id`t0`t28`t0`t106432`t64`t0`t106`t0`t108`t0"){throw "Native lane summon mismatch $id"}}
foreach($expected in @("EFFECT`t210880`t1`t179`t0`t6837`t0`t0`t47`t0`t18`t0","EFFECT`t210975`t0`t6`t395`t6840`t0`t0`t1`t0`t0`t0","SUMMON`t210976`t64`t0`t0`t0`t0")){if($native -notcontains $expected){throw "Missing native wind data $expected"}}
if($VerifyReleaseBinary){
 $oldPython=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$oldPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_titanstrike_storms_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Deployed storm callback verification failed'}}finally{$env:PYTHONPATH=$oldPython}
}
'PASS: native 7.3.5 storm data, production callbacks and repeatable SQL. Client wind visuals, movement, boarding and drop remain unverified.'
