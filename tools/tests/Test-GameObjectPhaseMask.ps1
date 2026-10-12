$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/gameobject-phase-mask"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=Get-Content "$repo/src/server/game/Globals/ObjectMgr.cpp" -Raw
$loader=$source.Substring($source.IndexOf('void ObjectMgr::LoadGameobjects()'))
$statement=[regex]::Match($loader,'data\.phaseMask\s*=\s*fields\[18\]\.GetUInt\d+\(\);')
if(!$statement.Success){throw 'Missing production phaseMask read'}
[IO.File]::WriteAllText("$output/GameObjectPhaseMask.inc",$statement.Value)
$data=Get-Content "$repo/src/server/game/Entities/GameObject/GameObjectData.h" -Raw
if($data -notmatch 'uint32 phaseMask = 1;'){throw 'GameObjectData phaseMask type changed'}
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@([regex]::Matches($group.ClCompile.AdditionalOptions,'/external:I "([^"]+)"') | ForEach-Object {$_.Groups[1].Value})
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++20','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=(@($output)+$includes | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='"'+$repo+'/tools/tests/GameObjectPhaseMaskTest.cpp"'
$compilerArgs+='/Fe"'+$output+'/GameObjectPhaseMaskTest.exe"'
$compilerArgs+='/Fo"'+$output+'/GameObjectPhaseMaskTest.obj"'
$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-test.cmd",$cmd)
& "$output/build-test.cmd"
if($LASTEXITCODE){throw 'Phase mask regression build failed'}

$previousPath=$env:PATH
try {
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 & "$output/GameObjectPhaseMaskTest.exe"
 if($LASTEXITCODE){throw 'Phase mask regression failed'}
} finally {$env:PATH=$previousPath}
