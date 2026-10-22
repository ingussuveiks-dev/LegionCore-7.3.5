param([switch]$VerifyReleaseBinary,[string]$ReleaseDirectory="$PSScriptRoot/../../build-extractors/bin/Release")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/boost-action-bars"
New-Item -ItemType Directory -Force $out | Out-Null
$source=Get-Content "$repo/src/server/game/BattlePay/CharacterService.cpp" -Raw
$a=$source.IndexOf('bool CharacterService::ArrangeBoostActionBar(')
$b=$source.IndexOf('CharacterService* CharacterService::instance()', $a)
if($a -lt 0 -or $b -le $a){throw 'Production boost action arranger missing'}
[IO.File]::WriteAllText("$out/Arrange.inc",$source.Substring($a,$b-$a))
$tutorial=Get-Content "$repo/src/server/scripts/Scenario/BoostExperience/boost_experience.cpp" -Raw
$phase=$tutorial.IndexOf('else if (phase == 2)')
$next=$tutorial.IndexOf('else', $phase+8)
$departure=$tutorial.Substring($phase,$next-$phase)
if(!$departure.Contains('sCharacterService->ArrangeBoostActionBar(player);') -or $departure.Contains('player->SaveToDB();')){throw 'Arrange before leaving without forcing an airborne position checkpoint'}
$mapChanged=$tutorial.Substring($tutorial.IndexOf('class player_boost_tutorial_bars'))
if($mapChanged.Contains('ArrangeBoostActionBar')){throw 'Ordinary map change must not reset personal layouts'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$profile="$repo/src/server/game/BattlePay"
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" /I`"$profile`" `"$PSScriptRoot/BoostActionBarsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native boost action arranger regression failed'}
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@($profile,"$repo/src/common/collision/Management","$repo/src/common/collision","$repo/dep/g3dlite/include",'C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$args=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$args+=($includes | ForEach-Object {'/I"'+$_+'"'})
$args+='"'+$PSScriptRoot+'/BoostActionBarCatalog.cpp"';$args+='/Fe"'+$out+'/catalog.exe"';$args+='/Fo"'+$out+'/catalog.obj"';$args+='/link';$args+=($libs | ForEach-Object {'"'+$_+'"'})
[IO.File]::WriteAllText("$out/catalog.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($args -join ' ')+"`nexit /b %errorlevel%`n")
& "$out/catalog.cmd"
if($LASTEXITCODE){throw 'Native action bar catalog compilation failed'}
$oldPath=$env:PATH
try{
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 & "$out/catalog.exe" "$repo/build-extractors/bin/Release/dbc/enUS/" | Set-Content "$out/catalog.tsv"
 if($LASTEXITCODE){throw 'Starter profile spell absent from native 7.3.5 data'}
}finally{$env:PATH=$oldPath}
'PASS: all profile/utility spell IDs exist in native 7.3.5 Spell.db2; names/attributes saved to catalog.tsv'
if($VerifyReleaseBinary){
 $python='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
 & $python "$PSScriptRoot/verify_boost_action_bars_release.py" $ReleaseDirectory
 if($LASTEXITCODE){throw 'Linked boost action layout verification failed'}
}
