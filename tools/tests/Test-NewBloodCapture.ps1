$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/new-blood-capture"
New-Item -ItemType Directory -Force -Path $output | Out-Null
function Export-Method($path,$start,$end,$out){
 $s=Get-Content "$repo/$path" -Raw;$a=$s.IndexOf($start);$b=$s.IndexOf($end,$a)
 if($a -lt 0 -or $b -lt 0){throw "Missing production method: $start"}
 [IO.File]::WriteAllText("$output/$out",$s.Substring($a,$b-$a))
}
Export-Method 'src/server/game/Conditions/ConditionMgr.cpp' 'bool ConditionMgr::IsObjectMeetingSmartEventConditions(' 'bool Condition::isLoaded()' 'ConditionsCall.inc'
Export-Method 'src/server/game/Spells/SpellEffects.cpp' 'void Spell::EffectKnockBack(' 'void Spell::EffectLeapBack(' 'Knockback.inc'
$migration=Get-Content "$repo/sql/updates/world/2026_10_13_05_new_blood_capture_and_portal.sql" -Raw
$fixture=(@('conditions','smart_scripts') | ForEach-Object {"CREATE TEMPORARY TABLE saved_$_ LIKE $_; INSERT INTO saved_$_ SELECT * FROM $_; CREATE TEMPORARY TABLE $_ LIKE saved_$_; INSERT INTO $_ SELECT * FROM saved_$_;"}) -join "`n"
$fixture+="`n$migration`n$migration`n"
$fixture+=@"
SELECT CONCAT('{',ConditionTarget,',',ConditionValue1,'},') FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=98831 AND SourceGroup=3 AND ConditionTypeOrReference=22 ORDER BY ElseGroup;
SELECT 'base-map-guards',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=98831 AND SourceGroup IN(3,5) AND ConditionTypeOrReference=22 AND ConditionTarget=1;
SELECT 'stun-release',COUNT(*) FROM smart_scripts t JOIN smart_scripts a ON a.entryorguid=101460 AND a.source_type=0 AND a.id=t.link WHERE t.entryorguid=101460 AND t.source_type=9 AND t.id=0 AND t.action_type=85 AND t.action_param2=2 AND a.action_type=28 AND a.action_param1=204833 AND a.target_type=7;
SELECT 'ground-portal',COUNT(*) FROM smart_scripts WHERE entryorguid=98797 AND source_type=0 AND id=2 AND link=4 AND target_z BETWEEN 696.65 AND 696.67;
SELECT 'early-interior',COUNT(*) FROM smart_scripts WHERE entryorguid=98797 AND source_type=0 AND action_type=50 AND action_param1=245958;
SELECT 'click-interior',COUNT(*) FROM smart_scripts WHERE entryorguid=101433 AND source_type=0 AND id=6 AND event_type=61 AND action_type=50 AND action_param1=245958;
SELECT 'ready-delay',COUNT(*) FROM smart_scripts WHERE entryorguid=101433 AND source_type=0 AND id=7 AND event_param1=13000 AND event_param2=13000 AND link=8 AND action_type=128 AND action_param1=0;
SELECT 'ready-click',COUNT(*) FROM smart_scripts WHERE entryorguid=101433 AND source_type=0 AND id=8 AND action_type=81 AND action_param1=16777216;
SELECT 'reignite',COUNT(*) FROM smart_scripts t JOIN smart_scripts a ON a.entryorguid=98797 AND a.source_type=0 AND a.id=t.link WHERE t.entryorguid=9879701 AND t.source_type=9 AND t.id=9 AND a.event_type=61 AND a.action_type=75 AND a.action_param1=199994 AND a.target_param1=101433;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=$fixture | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names;if($LASTEXITCODE){throw 'SQL fixture failed'}}finally{$env:MYSQL_PWD=$old}
$conditions=@($rows | Where-Object {$_.StartsWith('{')})
if($conditions.Count -ne 2){throw 'Missing migrated capture conditions'}
[IO.File]::WriteAllText("$output/CaptureConditions.inc",($conditions -join "`n"))
$counts=@($rows | Where-Object {-not $_.StartsWith('{')})
$expected=@('base-map-guards:4','stun-release:1','ground-portal:1','early-interior:0','click-interior:1','ready-delay:1','ready-click:1','reignite:1') | ForEach-Object {$_.Replace(':',"`t")}
if(($counts -join '|') -ne ($expected -join '|')){throw "Unexpected ritual data: $counts"}
$map=Get-Content "$repo/src/server/game/Maps/Map.cpp" -Raw
$a=$map.IndexOf('if (entry == 101433 && summon->HasQuestObjectScope())')
$b=$map.IndexOf('AddToMap(summon->ToCreature());',$a)
if($a -lt 0 -or $b -lt 0 -or $map.Substring($a,$b-$a) -notmatch 'SetAnimKitId\(1254\)' -or $map.Substring($a,$b-$a) -notmatch 'SetDisableGravity\(true\)'){throw 'Portal create packet lacks animation/gravity setup'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`n"
foreach($name in @('NewBloodCaptureTest','NewBloodKnockbackTest')){
 $cmd+="cl /nologo /EHsc /std:c++17 /W4 /I`"$output`" `"$PSScriptRoot/$name.cpp`" /Fe`"$output/$name.exe`" /Fo`"$output/$name.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$output/$name.exe`"`r`nif errorlevel 1 exit /b 1`r`n"
}
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Production handler regression failed'}
'PASS: repeatable migration, portal assembly/interaction timing and capture guards. Client visuals still require retesting.'
