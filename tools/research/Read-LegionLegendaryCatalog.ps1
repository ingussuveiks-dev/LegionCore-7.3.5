param([string]$OutputFile="",[string]$DbcDirectory="",[string]$SourceFile='LegionLegendaryCatalog.cpp',[string]$ProbeName='LegionLegendaryCatalog')
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo "build-extractors/runtime-test/legion-legendary-catalog"
New-Item -ItemType Directory -Force -Path $output | Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$commonIncludes=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$args=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$args+=($includes+$commonIncludes+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include') | ForEach-Object {'/I"'+$_+'"'})
$args+='"'+$repo+'/tools/research/'+$SourceFile+'"'
$args+='/Fe"'+$output+'/'+$ProbeName+'.exe"'
$args+='/Fo"'+$output+'/'+$ProbeName+'.obj"'
$args+='/link';$args+=($libs | ForEach-Object {'"'+$_+'"'})
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($args -join ' ')+"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/build-item.cmd",$cmd)
& "$output/build-item.cmd"
if($LASTEXITCODE){throw 'Native ItemSparse probe build failed'}

$previousPath=$env:PATH
try {
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 if(!$DbcDirectory){$DbcDirectory="$repo/build-extractors/bin/Release/dbc/enUS/"}
 $catalog=& "$output/$ProbeName.exe" ($DbcDirectory.TrimEnd('/','\')+'/')
 if($LASTEXITCODE){throw 'Native legendary catalog contains an unresolved item spell or failed to load'}
 if($OutputFile){$catalog | Set-Content -Encoding utf8 $OutputFile}else{$catalog}
} finally {$env:PATH=$previousPath}
