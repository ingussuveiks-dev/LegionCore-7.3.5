param([switch]$Baseline)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$out="$repo/build-extractors/runtime-test/world-socket-start-thread"
New-Item -ItemType Directory -Force $out | Out-Null
$source=if($Baseline){$s=& git -C $repo show 'f27fd982:src/server/game/Server/WorldSocket.cpp';if($LASTEXITCODE){throw 'Pinned baseline unavailable'};$s -join "`n"}else{[IO.File]::ReadAllText("$repo/src/server/game/Server/WorldSocket.cpp")}
function Export-Function([string]$marker,[string]$file){
 $a=$source.IndexOf($marker);if($a -lt 0){throw "Missing $marker"};$b=$source.IndexOf('{',$a)+1;$depth=1
 while($depth -gt 0 -and $b -lt $source.Length){if($source[$b] -eq '{'){++$depth};if($source[$b] -eq '}'){--$depth};++$b}
 if($depth){throw "Unbalanced $marker"}
 [IO.File]::WriteAllText("$out/$file",$source.Substring($a,$b-$a))
}
Export-Function 'void WorldSocket::Start()' 'Start.inc'
Export-Function 'void WorldSocket::CheckIpCallback(' 'CheckIp.inc'
if(!$source.Contains('_queryProcessor.ProcessReadyCallbacks();')){throw 'Network Update no longer pumps the callback processor'}
if(!$Baseline){
 $db=[IO.File]::ReadAllText("$repo/src/server/database/Database/Implementation/LoginDatabase.cpp")
 if($db -notmatch 'PrepareStatement\(LOGIN_SEL_IP_INFO, "SELECT \* FROM ip_banned WHERE ip = \?", CONNECTION_BOTH\)'){throw 'IP query must support both async world and synchronous bnet callers'}
}
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@($out,"$repo/src/server/shared/Networking",'C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$args=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
if($Baseline){$args+='/DEXPECT_BASELINE'}
$args+=($includes | ForEach-Object {'/I"'+$_+'"'})
$args+='"'+$PSScriptRoot+'/WorldSocketStartThreadTest.cpp"';$args+='/Fe"'+$out+'/test.exe"';$args+='/Fo"'+$out+'/test.obj"';$args+='/link';$args+=($libs | ForEach-Object {'"'+$_+'"'})
[IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($args -join ' ')+"`nexit /b %errorlevel%`n")
& "$out/build.cmd";if($LASTEXITCODE){throw 'Native socket-start probe build failed'}
$oldPath=$env:PATH
try{$env:PATH="$repo/build-extractors/bin/Release;$env:PATH";& "$out/test.exe";if($LASTEXITCODE){throw 'Socket-start ownership regression failed'}}finally{$env:PATH=$oldPath}
