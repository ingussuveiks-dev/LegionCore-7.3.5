$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/vehicle-accessory-boarding"
New-Item -ItemType Directory -Force $out | Out-Null
function Block([string]$source,[string]$marker){$a=$source.IndexOf($marker);if($a -lt 0){throw "Missing production $marker"};$b=$source.IndexOf('{',$a);$end=$b+1;$depth=1;while($depth -and $end -lt $source.Length){if($source[$end] -eq '{'){$depth++};if($source[$end] -eq '}'){$depth--};$end++};if($depth){throw 'Unbalanced production handler'};$source.Substring($a,$end-$a)}
[IO.File]::WriteAllText("$out/Install.inc",(Block (Get-Content "$repo/src/server/game/Entities/Vehicle/Vehicle.cpp" -Raw) 'void Vehicle::InstallAccessory(VehicleAccessory const* as)'))
$enter=Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw) 'void Unit::EnterVehicle('
$enter=$enter.Replace('bool fullTriggered /*= false*/','bool fullTriggered')
[IO.File]::WriteAllText("$out/Enter.inc",$enter)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/VehicleAccessoryBoardingTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Vehicle accessory boarding regression failed'}
$migration=Get-Content "$repo/sql/updates/world/2026_10_21_08_grasp_scripted_accessory.sql" -Raw
$sql=@"
CREATE TEMPORARY TABLE dht_saved_accessory LIKE vehicle_template_accessory;
INSERT INTO dht_saved_accessory SELECT * FROM vehicle_template_accessory WHERE EntryOrAura IN(97648,114970);
CREATE TEMPORARY TABLE vehicle_template_accessory LIKE dht_saved_accessory;
INSERT INTO vehicle_template_accessory SELECT * FROM dht_saved_accessory;
REPLACE INTO vehicle_template_accessory (EntryOrAura,accessory_entry,seat_id,offsetX,offsetY,offsetZ,offsetO,minion,description,summontype,summontimer)
VALUES (97648,95051,0,-1.96758,-0.135845,-0.384596,0,1,'Broken untracked captive',8,0);
$migration
$migration
SELECT 'OLD',COUNT(*) FROM vehicle_template_accessory WHERE EntryOrAura=97648 AND accessory_entry=95051 AND seat_id=0;
SELECT 'GONDOLA',COUNT(*) FROM vehicle_template_accessory WHERE EntryOrAura=114970;
SELECT 'CLICK',npc_entry,spell_id,cast_flags FROM npc_spellclick_spells WHERE npc_entry IN(97648,114970) ORDER BY npc_entry;
SELECT 'AI',ScriptName FROM creature_template WHERE entry=97648;
SELECT 'VEHICLE',entry,VehicleId FROM creature_template WHERE entry IN(97648,114970) ORDER BY entry;
"@
$config=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$config[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($config[0]) --port=$($config[1]) --user=$($config[2]) --database=$($config[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Vehicle accessory SQL replay failed'}}finally{$env:MYSQL_PWD=$previous}
foreach($expected in @("OLD`t0","GONDOLA`t7","CLICK`t97648`t186608`t1","CLICK`t114970`t230986`t1","AI`tnpc_grasp_of_underking_quest","VEHICLE`t97648`t4397","VEHICLE`t114970`t5016")){if($rows -notcontains $expected){throw "Accessory content regression: $expected"}}
'PASS: repeatable scoped deletion preserves seven Gondola passengers, both player-click spells and scripted Riverbend captive ownership.'
# Native spell data confirms the player actions are not direct boarding auras.
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile VehicleAccessoryCatalog.cpp -ProbeName VehicleAccessoryCatalog -OutputFile "$out/native-catalog.tsv"
$catalog=@(Get-Content "$out/native-catalog.tsv")
$ride=@($catalog | Where-Object {$_ -like "E`t46598`t0`t0`t*"})
if($ride.Count -ne 1 -or ($ride[0] -split "`t")[5] -ne '236'){throw 'Native hardcoded ride no longer uses CONTROL_VEHICLE'}
foreach($id in @(186608,230986)){if($catalog | Where-Object {$_ -like "E`t$id`t*" -and ($_ -split "`t")[5] -eq '236'}){throw "Review: click $id now contains a direct boarding aura"}}
foreach($index in 1..7){if(!($catalog | Where-Object {$_.StartsWith("SEAT`t5016`t$index`t")})){throw "Gondola native seat $index is missing"}}
if(!($catalog | Where-Object {$_.StartsWith("SEAT`t4397`t0`t")})){throw 'Grasp native seat zero is missing'}
'PASS: production native DB2 reader confirms 46598 control aura, non-boarding player clicks, seven real Gondola seats and Grasp seat zero.'
