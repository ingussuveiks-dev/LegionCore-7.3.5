param([string]$MigrationFile="$PSScriptRoot/../../sql/updates/world/2026_10_15_32_dark_whispers_finale.sql",[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/dark-whispers"
New-Item -ItemType Directory -Force -Path $out | Out-Null
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile DarkWhispersCatalog.cpp -ProbeName DarkWhispersCatalog -OutputFile "$out/native.tsv"
$native=Get-Content "$out/native.tsv"
foreach($expected in @('step 2478 4 48050','step 2479 5 48053','tree 48051 48050 1 0 30107 92 50961','tree 48052 48050 1 0 30108 92 50962','tree 48054 48053 1 0 30109 92 50945')){
    if($native -notcontains $expected){throw "Native scenario route changed: $expected"}
}
function Extract([string]$s,[string]$start,[string]$end){$a=$s.IndexOf($start);$b=$s.IndexOf($end,$a);if($a -lt 0 -or $b -lt 0){throw "Missing production section: $start"};return $s.Substring($a,$b-$a)}
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/dark_whispers_artifact.cpp" -Raw
[IO.File]::WriteAllText("$out/ProtectFinale.inc",(Extract $source 'namespace DarkWhispersArtifact' 'class instance_dark_whispers_artifact'))
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
[IO.File]::WriteAllText("$out/NativeScaledHook.inc",(Extract $source '    damage /= victim->GetHealthMultiplierForTarget(this);' '    if (!victim->IsControlledByPlayer()'))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
[IO.File]::WriteAllText("$out/SmartDamage.inc",(Extract $source 'void SmartAI::DamageTaken(' 'void SmartAI::HealReceived('))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
$start=$source.IndexOf('void SmartScript::ProcessEvent(')
[IO.File]::WriteAllText("$out/HealthEvent.inc",(Extract $source.Substring($start) '        case SMART_EVENT_UPDATE_IC:' '        case SMART_EVENT_TARGET_HEALTH_PCT:'))
$enums=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
[IO.File]::WriteAllText("$out/EventFlags.inc",(Extract $enums 'enum SmartEventFlags' 'enum SmartCastFlags'))
if(!$source.Contains('if (failedSpellCast && !successfulSpellCast)')){throw 'Review native cast failure semantics'}
$migration=Get-Content $MigrationFile -Raw
$fixture=@"
CREATE TEMPORARY TABLE saved_sai LIKE smart_scripts;
INSERT INTO saved_sai SELECT * FROM smart_scripts WHERE entryorguid IN(106652,106693,249821) AND source_type IN(0,1);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_sai;
INSERT INTO smart_scripts SELECT * FROM saved_sai;
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=106652;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
CREATE TEMPORARY TABLE saved_instance LIKE instance_template;
CREATE TEMPORARY TABLE instance_template LIKE saved_instance;
INSERT INTO instance_template SELECT * FROM saved_instance WHERE map=1498;
"@
$select=@"
SELECT entryorguid,id,link,event_type,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,target_type,target_param1,target_param2,target_x,target_y,target_z FROM smart_scripts ORDER BY entryorguid,source_type,id;
SELECT 'condition',SourceGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2 FROM conditions ORDER BY SourceGroup;
SELECT 'instance',map,script FROM instance_template;
"@
$fixture+="`n$migration`nSELECT 'PASS1';`n$select`n$migration`nSELECT 'PASS2';`n$select`nUPDATE instance_template SET script='unrelated_custom_instance' WHERE map=1498;`n$migration`nSELECT 'custom',COUNT(*) FROM instance_template WHERE script='unrelated_custom_instance';"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($fixture | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Dark Whispers SQL fixture failed'}}finally{$env:MYSQL_PWD=$old}
$a=[Array]::IndexOf($rows,'PASS1');$b=[Array]::IndexOf($rows,'PASS2');$d=[Array]::IndexOf($rows,"custom`t1")
if($a -lt 0 -or $b -lt 0 -or $d -lt 0){throw 'Fixture markers/custom instance preservation failed'}
$first=@($rows[($a+1)..($b-1)]);$second=@($rows[($b+1)..($d-1)])
if(($first -join "`n") -ne ($second -join "`n")){throw 'Migration replay changed rows'}
foreach($group in 3,4,5){if($first -notcontains "condition`t$group`t44`t1`t1097`t4"){throw 'Missing base-actor stage condition'}}
if($first -notcontains "instance`t1498`tinstance_dark_whispers_artifact"){throw 'Missing damage hook binding'}
[IO.File]::WriteAllLines("$out/rows.tsv",[string[]]@($first | Where-Object {$_ -match '^\d'}))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/DarkWhispersFinaleTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/rows.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Dark Whispers native damage/health finale regression failed'}
'PASS: SQL replay, actor stage gates, custom instance preservation'
if($VerifyReleaseBinary){
    $binary=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes("$repo/build-extractors/bin/Release/worldserver.exe"))
    foreach($marker in @('Dark Whispers: protected Mephistroth', 'instance_dark_whispers_artifact')){if(!$binary.Contains($marker)){throw "Canonical binary lacks $marker"}}
    'PASS: canonical Release contains Dark Whispers instance and post-scaling protection'
}
