param([Parameter(Mandatory)][string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$release = "$repo/build-extractors/bin/Release"
$out = (New-Item -ItemType Directory -Path $OutputDirectory -Force).FullName
$project = "$repo/build-extractors/src/server/worldserver"
[xml]$config = Get-Content "$project/worldserver.vcxproj"
$releaseGroup = $config.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes = $releaseGroup.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object { $_ -and $_ -notlike '%*' }
$collision = @('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include',
    'dep/recastnavigation/Detour/Include') | ForEach-Object { Join-Path $repo $_ }
$libs = $releaseGroup.Link.AdditionalDependencies.Split(';') | Where-Object { $_ -and $_ -notlike '%*' } | ForEach-Object {
    if ($_ -like '..*') { [IO.Path]::GetFullPath((Join-Path $project $_)) } else { $_ }
}
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$flags = @('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$flags += ($includes + $collision + @('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include',
    'C:/Program Files/OpenSSL-Win64/include') | ForEach-Object { '/I"' + $_ + '"' })
$flags += @(('"' + "$PSScriptRoot/MageTowerRuneGeometry.cpp" + '"'), ('/Fe"' + "$out/test.exe" + '"'),
    ('/Fo"' + "$out/test.obj" + '"'), '/link')
$flags += ($libs | ForEach-Object { '"' + $_ + '"' })
$cmd = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
cl $($flags -join ' ')
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$out/build-test.cmd", $cmd)
& "$out/build-test.cmd"
if ($LASTEXITCODE) { throw 'Could not build the geometry regression' }
$previous = $env:PATH
try {
    $env:PATH = "$release;$previous"
    & "$out/test.exe" $release
    if ($LASTEXITCODE) { throw "Twins arena geometry failed with exit $LASTEXITCODE" }
} finally {
    $env:PATH = $previous
}
