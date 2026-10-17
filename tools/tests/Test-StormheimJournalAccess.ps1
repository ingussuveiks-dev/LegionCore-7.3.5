$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-journals"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$source=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
$begin=$source.IndexOf('bool GameObject::IsInRange(');$end=$source.IndexOf('void GameObject::EventInform(',$begin)
if($begin -lt 0 -or $end -lt $begin){throw 'Native GO range extraction failed'}
[IO.File]::WriteAllText("$out/JournalAccessRange.inc",$source.Substring($begin,$end-$begin).Replace('GameObject::','AccessObject::'))
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$includes+=@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include',$out)
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='"'+$PSScriptRoot+'/StormheimJournalAccess.cpp"';$compilerArgs+='/Fe"'+$out+'/StormheimJournalAccess.exe"';$compilerArgs+='/Fo"'+$out+'/StormheimJournalAccess.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
[IO.File]::WriteAllText("$out/build-access.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n")
& "$out/build-access.cmd"
if($LASTEXITCODE){throw 'Native journal access build failed'}
$previousPath=$env:PATH
try{
 $runtime="$repo/build-extractors/bin/Release";$env:PATH="$runtime;$env:PATH"
 $rows=@(& "$out/StormheimJournalAccess.exe" $runtime "$PSScriptRoot/data/stormheim-halvir-walk.txt");$code=$LASTEXITCODE
 $rows | Set-Content "$out/journal-access.txt";$rows
 if($code){throw "Native journal access failed ($code)"}
 # A clear endpoint alone must not admit an unwalked gap. Keep only the two
 # valid end samples: native collision validation must reject the missing walk.
 $samples=@(Get-Content "$PSScriptRoot/data/stormheim-halvir-walk.txt" | Where-Object {$_ -and !($_.StartsWith('#'))})
 @($samples[0],$samples[-1]) | Set-Content "$out/missing-walk.txt"
 $rejected=@(& "$out/StormheimJournalAccess.exe" $runtime "$out/missing-walk.txt");$code=$LASTEXITCODE
 if($code -ne 5){throw "Missing walk was not rejected by physical segment validation ($code)"}
 $global:LASTEXITCODE=0
 'PASS: disconnected fixture endpoints cannot substitute for the collision walk.'
}finally{$env:PATH=$previousPath}
