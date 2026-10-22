param([switch]$VerifyReleaseBinary,[string]$ReleaseDirectory="$PSScriptRoot/../../build-extractors/bin/Release")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/boost-construct-lessons"
New-Item -ItemType Directory -Force $out | Out-Null
$source=Get-Content "$repo/src/server/scripts/Scenario/BoostExperience/boost_experience.cpp" -Raw
function Block([string]$marker){
 $a=$source.IndexOf($marker);if($a -lt 0){throw "Missing production $marker"};$b=$source.IndexOf('{',$a)+1;$depth=1
 while($depth -and $b -lt $source.Length){if($source[$b] -eq '{'){$depth++};if($source[$b] -eq '}'){$depth--};$b++}
 $source.Substring($a,$b-$a).Replace(' override','')
}
foreach($entry in @(
 @('CombatStage','enum class CombatStage'),@('Combat','constexpr CombatStage GetCombatStage('),@('Surrender','constexpr bool ShouldSparringOpponentSurrender('),
 @('Criterion','bool HasLessonCriterion('),@('Needs','bool NeedsArcaneConstructs('),@('First','uint8 GetFirstTargetLesson('),@('Offset','Position Offset('),
 @('Clear','void ClearConstructWave('),@('Spawn','void SetArcaneConstructsForStep('),@('Death','bool HandleConstructDeath('),@('Retry','void UpdateConstructLesson('),
 @('Dummy','void SetTrainingDummyForStep('),@('Damage','void OnCreatureDamageTaken('),@('ActiveSpar','bool IsActiveSparringOpponent('),
 @('SparCredit','void SetGuidData('),@('CreatureDies','void CreatureDies('),@('Next','void onScenarionNextStep(')
)){
 $block=Block $entry[1]
 if($entry[0] -eq 'CombatStage'){$block+=';'}
 [IO.File]::WriteAllText("$out/$($entry[0]).inc",$block)
}
if($source -notmatch 'void Update\(uint32 diff\) override\s*\{\s*UpdateConstructLesson\(diff\);'){throw 'Native instance update does not run construct retry lifecycle'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/BoostConstructLessonsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif %errorlevel% neq 0 exit /b %errorlevel%`n`"$out/test.exe`"`nif %errorlevel% neq 0 exit /b %errorlevel%`ncl /nologo /EHsc /std:c++17 /I`"$repo/src/server/game/Entities/Creature`" `"$PSScriptRoot/CreatureScalingTest.cpp`" /Fe`"$out/scaling.exe`" /Fo`"$out/scaling.obj`"`nif %errorlevel% neq 0 exit /b %errorlevel%`n`"$out/scaling.exe`"`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native boost construct lifecycle regression failed'}
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@($out,"$repo/src/common/collision/Management","$repo/src/common/collision","$repo/dep/g3dlite/include",'C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='"'+$PSScriptRoot+'/BoostConstructCatalog.cpp"';$compilerArgs+='/Fe"'+$out+'/catalog.exe"';$compilerArgs+='/Fo"'+$out+'/catalog.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
[IO.File]::WriteAllText("$out/catalog.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n")
& "$out/catalog.cmd"
if($LASTEXITCODE){throw 'Native boost construct catalog build failed'}
$oldPath=$env:PATH
try{$env:PATH="$repo/build-extractors/bin/Release;$env:PATH";& "$out/catalog.exe" "$repo/build-extractors/bin/Release/dbc/enUS/";if($LASTEXITCODE){throw 'Native boost construct DB2 check failed'}}finally{$env:PATH=$oldPath}
$sql="SELECT COUNT(*) FROM creature_template c JOIN creature_template_wdb w ON w.Entry=c.entry WHERE c.entry=108478 AND c.minlevel=100 AND c.maxlevel=100 AND c.faction=14 AND w.DisplayID1=56111 AND w.HpMulti>0 AND (c.unit_flags & 33554690)=0 AND c.ScriptName<>'npc_boost_training_dummy';"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Construct template check failed'}}finally{$env:MYSQL_PWD=$old}
if($rows -notcontains '1'){throw 'Construct template is missing, unattackable or bound to immortal-dummy AI'}
if($VerifyReleaseBinary){
 $python=(Get-Command python -ErrorAction SilentlyContinue | Select-Object -First 1).Source
 if(!$python){$python='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'}
 & $python "$PSScriptRoot/verify_boost_construct_release.py" $ReleaseDirectory
 if($LASTEXITCODE){throw 'Linked boost construct implementation verification failed'}
}
