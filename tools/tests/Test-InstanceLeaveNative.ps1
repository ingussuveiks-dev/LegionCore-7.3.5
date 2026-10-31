param([switch]$Baseline,[string]$OutputDirectory='',[string]$BaselineDirectory='')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/instance-leave-native$(if($Baseline){'-baseline'})"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup|Where-Object Condition -like '*Release|x64*'|Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';')|Where-Object {$_ -and $_ -notlike '%*'}
$libs=$group.Link.AdditionalDependencies.Split(';')|Where-Object {$_ -and $_ -notlike '%*'}|ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$game=$libs|Where-Object {$_ -match 'game[\\/]Release[\\/]game.lib$'}|Select-Object -First 1
$scripts=$libs|Where-Object {$_ -match 'scripts[\\/]Release[\\/]scripts.lib$'}|Select-Object -First 1
if(!$game){throw 'Missing actual final-link game library'}
if($Baseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/instance-leave-baseline-ba856f49b"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne 'ba856f49b' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned counterfactual library hash differs'}
 if((Get-FileHash "$BaselineDirectory/scripts.lib").Hash -ne $manifest.scripts){throw 'Pinned scripts library differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}elseif($_ -eq $scripts){"$BaselineDirectory/scripts.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
 $scripts="$BaselineDirectory/scripts.lib"
}else{
 foreach($file in @('Maps/Map.cpp')){
  if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
 }
 foreach($file in @('Pandaria/HeartFear/instance_heart_of_fear.cpp','Pandaria/SiegeofOrgrimmar/instance_siege_of_orgrimmar.cpp')){
  if((Get-Item "$repo/src/server/scripts/$file").LastWriteTimeUtc -gt (Get-Item $scripts).LastWriteTimeUtc){throw "Actual scripts library predates $file"}
 }
}
$questSource=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
$query=[regex]::Match($questSource,'(?s)void QuestDataStoreMgr::LoadQuests\(\).*?WorldDatabase.Query\(("SELECT ".*?" FROM quest_template")\);').Groups[1].Value
if(!$query){throw 'Native quest loader SELECT missing'}
[IO.File]::WriteAllText("$out/NativeLostArmyQuest.inc",$query+' " WHERE ID IN(44923,45088)"')
$extra=@('src/server/scripts/Legion','src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include')|ForEach-Object {Join-Path $repo $_}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$language=switch($group.ClCompile.LanguageStandard){'stdcpp14'{'/std:c++14'}'stdcpp17'{'/std:c++17'}'stdcpp20'{'/std:c++20'}'stdcpplatest'{'/std:c++latest'}default{throw 'Unsupported native language'}}
if($group.ClCompile.RuntimeLibrary -ne 'MultiThreadedDLL'){throw 'Expected actual Release /MD ABI'}
$compilerArgs=@('/nologo','/EHsc',$language,'/MD','/DNDEBUG','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
if($Baseline){$compilerArgs+='/DINSTANCE_LEAVE_BASELINE'}
$compilerArgs+=($includes+$extra+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include',$out)|ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='"'+$PSScriptRoot+'/InstanceLeaveNative.cpp"'
$compilerArgs+=@('/Fe"'+$out+'/probe.exe"','/Fo"'+$out+'/probe.obj"','/link','/DEBUG','/MAP:"'+$out+'/probe.map"','/PDB:"'+$out+'/probe.pdb"')
$compilerArgs+=($libs|ForEach-Object {'"'+$_+'"'})
[IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n")
& "$out/build.cmd";if($LASTEXITCODE){throw 'Native instance leave build failed'}
$runtime="$repo/build-extractors/bin/Release"
$run="$out/run-$(Get-Date -Format yyyyMMdd-HHmmss)";New-Item -ItemType Directory -Force $run|Out-Null
$config=Get-Content "$runtime/worldserver.conf" -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$run.Replace('\','/')+'"'))
[IO.File]::WriteAllText("$run/worldserver.conf",$config)
$prior=$env:PATH
try{
 $env:PATH="$runtime;$prior";Push-Location $runtime
 try{& "$out/probe.exe" "$run/worldserver.conf" "$runtime/" > "$run/stdout.log" 2> "$run/stderr.log";$exit=$LASTEXITCODE}finally{Pop-Location}
}finally{$env:PATH=$prior}
Get-Content "$run/stdout.log"|Where-Object {$_ -match '^(PASS|EXPECTED_BROKEN|CASE)'}
if($exit -or (Get-Item "$run/stderr.log").Length){Get-Content "$run/stderr.log" -Tail 12;throw "Native instance leave exit=$exit or stderr"}
if((Test-Path "$run/DBErrors.log") -and (Get-Item "$run/DBErrors.log").Length){throw 'Native leave emitted DBErrors; inspect preserved evidence'}
@{baseline=[bool]$Baseline;game=(Get-FileHash $game).Hash;scripts=(Get-FileHash $scripts).Hash;executable=(Get-FileHash "$out/probe.exe").Hash;source=(Get-FileHash "$PSScriptRoot/InstanceLeaveNative.cpp").Hash;run=$run;exit=$exit}|ConvertTo-Json|Set-Content "$out/native-inputs.json"
'PASS native execution/exit/stdout/stderr/DBErrors; wrapper must also inspect crash and appended server diagnostics'
