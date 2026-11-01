param([switch]$AddressSanitizer,[switch]$VerifyLiveData,[string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/fuzan-brewing-native")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 $quest=[IO.File]::ReadAllText("$repo/src/server/game/Globals/QuestData.cpp")
 $query=[regex]::Match($quest,'(?s)void QuestDataStoreMgr::LoadQuests\(\).*?WorldDatabase.Query\(("SELECT ".*?" FROM quest_template")\);').Groups[1].Value
 if(!$query){throw 'Actual native quest loader query missing'}
 foreach($pair in @(@('NativeFuZanQuest.inc',' WHERE ID=42765'),@('NativeLostArmyQuest.inc',' WHERE ID IN(44923,45088)'),@('NativeBrewingQuest.inc',' WHERE ID=42868'))){[IO.File]::WriteAllText("$out/$($pair[0])",$query+' "'+$pair[1]+'"')}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $group=$project.Project.ItemDefinitionGroup|Where-Object Condition -like '*Release|x64*'|Select-Object -First 1
 $includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';')|Where-Object {$_ -and $_ -notlike '%*'}
 $libs=$group.Link.AdditionalDependencies.Split(';')|Where-Object {$_ -and $_ -notlike '%*'}|ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
 $game=$libs|Where-Object {$_ -match 'game[\\/]Release[\\/]game.lib$'}|Select-Object -First 1
 $scripts=$libs|Where-Object {$_ -match 'scripts[\\/]Release[\\/]scripts.lib$'}|Select-Object -First 1
 if(!$game -or !$scripts -or $group.ClCompile.RuntimeLibrary -ne 'MultiThreadedDLL'){throw 'Native Release ABI/final-link inputs differ'}
 $inputs=@($PSCommandPath,"$PSScriptRoot/FuZanBrewingNative.cpp","$PSScriptRoot/FuZanTempleIntroNative.cpp","$PSScriptRoot/MasterLootNative.cpp","$PSScriptRoot/LostArmyLootNative.cpp","$PSScriptRoot/RockawayOffensiveNative.cpp","$PSScriptRoot/RockawayChannelNative.cpp","$repo/src/server/scripts/Scenario/Artifacts/Monk/TempleOfTheJadeSerpent/fuzan_brewing.h",$game,$scripts)
 $hashes=@{};foreach($path in $inputs){$hashes[$path]=(Get-FileHash -LiteralPath $path).Hash}
 $language=switch($group.ClCompile.LanguageStandard){'stdcpp17'{'/std:c++17'}'stdcpp20'{'/std:c++20'}'stdcpplatest'{'/std:c++latest'}default{throw 'Native language standard differs'}}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 if(!$vs){throw 'MSVC toolchain missing'}
 $extra=@('src/server/scripts/Legion','src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include')|ForEach-Object {Join-Path $repo $_}
 $args=@('/nologo','/EHsc',$language,'/MD','/DNDEBUG','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB','/DFUZAN_INTRO_LIVE')
 if($VerifyLiveData){$args+='/DFUZAN_BREWING_LIVE'}
 # Linked production libraries use the ordinary Release STL layout. Keep that
 # ABI; ASAN still instruments the newly compiled production script/fixture.
 if($AddressSanitizer){$args+=@('/fsanitize=address','/Zi','/D_DISABLE_VECTOR_ANNOTATION','/D_DISABLE_STRING_ANNOTATION')}
 $args+=($includes+$extra+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include',$out)|ForEach-Object {'/I"'+$_+'"'})
 $args+='"'+$PSScriptRoot+'/FuZanBrewingNative.cpp"'
 $args+=@('/Fe"'+$out+'/probe.exe"','/Fo"'+$out+'/probe.obj"','/Fd"'+$out+'/probe-compiler.pdb"','/link','/DEBUG','/INCREMENTAL:NO','/MAP:"'+$out+'/probe.map"','/PDB:"'+$out+'/probe.pdb"')
 $args+=($libs|ForEach-Object {'"'+$_+'"'})
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`nif errorlevel 1 exit /b 1`ncl "+($args -join ' ')+"`nexit /b %errorlevel%`n")
 & "$out/build.cmd" *> "$out/build.log";if($LASTEXITCODE){Get-Content "$out/build.log" -Tail 18;throw 'Native brewing build failed'}
 $runtime="$repo/build-extractors/bin/Release";$run="$out/run"
 New-Item -ItemType Directory -Force "$run/logs"|Out-Null
 $config=[regex]::Replace([IO.File]::ReadAllText("$runtime/worldserver.conf"),'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$run.Replace('\','/')+'/logs"'))
 $config=[regex]::Replace($config,'(?m)^Logger\.sql\.sql\s*=.*$','Logger.sql.sql=4,Console DBErrors')
 [IO.File]::WriteAllText("$run/worldserver.conf",$config)
 $prior=$env:PATH
 try{
  $compilerVersion=([IO.File]::ReadAllText("$vs/VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt")).Trim()
  $env:PATH="$runtime;$vs/VC/Tools/MSVC/$compilerVersion/bin/Hostx64/x64;$prior"
  & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/native-diagnostics" -RuntimeDirectories @($run) -Test {
   Push-Location $runtime
   try{& "$out/probe.exe" "$run/worldserver.conf" "$runtime/" > "$run/stdout.log" 2> "$run/stderr.log";$code=$LASTEXITCODE}finally{Pop-Location}
   @{exit=$code;stdout=(Get-FileHash "$run/stdout.log").Hash;stderr=(Get-FileHash "$run/stderr.log").Hash}|ConvertTo-Json|Set-Content "$run/process-result.json"
   $lines=@(Get-Content "$run/stdout.log");$lines|Where-Object {$_ -match '^(PASS|EOF)'}
   if($code -or (Get-Item "$run/stderr.log").Length){Get-Content "$run/stderr.log" -Tail 18;throw "Native brewing exit=$code or stderr"}
   if(@($lines|Where-Object {$_ -match '^PASS production brewing '}).Count -ne 4 -or $lines[-1] -cne 'EOF actual native brewing2'){throw 'Native brewing matrix incomplete'}
  }
 }finally{$env:PATH=$prior}
 foreach($path in $inputs){if((Get-FileHash -LiteralPath $path).Hash -ne $hashes[$path]){throw 'Native source/link input changed during execution'}}
 @{inputs=$hashes;executable=(Get-FileHash "$out/probe.exe").Hash;asan_script_fixture=[bool]$AddressSanitizer;native_core_release_not_instrumented=$true;private_metadata_projection=(!$VerifyLiveData);live_registrations=[bool]$VerifyLiveData;readonly_database_pools=4;authenticated_save=$false}|ConvertTo-Json -Depth 4|Set-Content "$out/native-inputs.json"
 'PASS native brewing matrix, exit/stdout/stderr and post-test diagnostics; client traversal remains separate'
}
