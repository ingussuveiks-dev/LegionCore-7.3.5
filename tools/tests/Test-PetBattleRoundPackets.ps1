param([switch]$AddressSanitizer,[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/pet-battle-round"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){if($s[$b] -eq '{'){++$depth};if($s[$b] -eq '}'){--$depth};++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($out,$repo,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 if($VerifyNativeCatalog){$oldConfig=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile PetBattleStateCatalog.cpp -ProbeName PetBattleStateCatalog -OutputFile "$out/catalog.tsv";Get-Content "$out/catalog.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$oldConfig}}
 $handler=Get-Content "$repo/src/server/game/Handlers/BattlePetHandler.cpp" -Raw
 foreach($p in @(@('First','void WorldSession::SendPetBattleFirstRound('),@('Round','void WorldSession::SendPetBattleRoundResult('),@('Replacement','void WorldSession::SendPetBattleReplacementMade('),@('Converter','static WorldPackets::BattlePet::Effect BuildPetBattleRoundEffect('))){[IO.File]::WriteAllText("$out/$($p[0]).inc",(Block $handler $p[1]))}
 $baseline=(& git -C $repo show '503fd969b:src/server/game/Handlers/BattlePetHandler.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned baseline missing'}
 [IO.File]::WriteAllText("$out/Baseline.inc",(Block $baseline 'void WorldSession::SendPetBattleRoundResult('))
 $header=Get-Content "$repo/src/server/game/PetBattle/PetBattle.h" -Raw
 [IO.File]::WriteAllText("$out/NativeStates.inc",(Block $header 'enum BattlePetState')+';')
 # The second declaration has its own line break; do not confuse EventUpdate.
 $event=[regex]::Match($header,'struct PetBattleEvent\s*\{').Value
 [IO.File]::WriteAllText("$out/NativeTypes.inc",(Block $header 'struct PetBattleEventUpdate')+';'+(Block $header $event)+';')
 $native=Get-Content "$repo/src/server/game/PetBattle/PetBattle.cpp" -Raw;$events=''
 foreach($name in @('PetBattleEventUpdate::PetBattleEventUpdate(','PetBattleEvent::PetBattleEvent(','PetBattleEvent& PetBattleEvent::UpdateHealth(','PetBattleEvent& PetBattleEvent::UpdateState(','PetBattleEvent& PetBattleEvent::UpdateFrontPet(','PetBattleEvent& PetBattleEvent::UpdateBuff(','PetBattleEvent& PetBattleEvent::UpdateSpeed(','PetBattleEvent& PetBattleEvent::Trigger(')){$events+=(Block $native $name)+"`n"}
 [IO.File]::WriteAllText("$out/NativeEvents.inc",$events)
 [IO.File]::WriteAllText("$out/NativeState.inc",(Block $native 'void PetBattle::SetPetState('))
 [IO.File]::WriteAllText("$out/NativeKill.inc",(Block $native 'void PetBattle::Kill('))
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $group=$project.Project.ItemDefinitionGroup|Where-Object Condition -like '*Release|x64*'|Select-Object -First 1
 $includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';')|Where-Object {$_ -and $_ -notlike '%*'}
 $libs=$group.Link.AdditionalDependencies.Split(';')|Where-Object {$_ -and $_ -notlike '%*'}|ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 # Match the existing Release libraries' STL ABI; extracted handlers remain
 # ASAN-instrumented, while those prebuilt libraries are not instrumented.
 $flags=if($AddressSanitizer){'/fsanitize=address /Zi /D_DISABLE_VECTOR_ANNOTATION /D_DISABLE_STRING_ANNOTATION'}else{''}
 $args=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB',$flags)
 $args+=($includes+@($out,"$repo/src/common/Collision/Management","$repo/src/common/Collision","$repo/dep/g3dlite/include","$repo/dep/recastnavigation/Detour/Include",'C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')|ForEach-Object {'/I"'+$_+'"'})
 $args+='"'+$PSScriptRoot+'/PetBattleRoundPacketsTest.cpp"';$args+='/Fe"'+$out+'/test.exe"';$args+='/Fo"'+$out+'/test.obj"';$args+='/Fd"'+$out+'/test-compiler.pdb"';$args+='/link';$args+=($libs|ForEach-Object {'"'+$_+'"'})
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($args -join ' ')+"`nif errorlevel 1 exit /b 1`nset PATH=$repo/build-extractors/bin/Release;%PATH%`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Pet battle native packet regression failed'}
}
