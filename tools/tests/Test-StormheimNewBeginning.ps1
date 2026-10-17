param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-StormheimNewBeginningData.ps1"
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-new-beginning"
& "$PSScriptRoot/Test-StormheimAscendant.ps1" *> "$out/ascendant-regression.log"
& "$PSScriptRoot/Test-MovieMovement.ps1" *> "$out/movie-movement.log"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-ascendant/*.inc" | Copy-Item -Destination $out
Copy-Item "$repo/build-extractors/runtime-test/movie-movement/MovieMethods.inc" $out
function Extract([string]$s,[string]$signature){$a=$s.IndexOf($signature);if($a -lt 0){throw "Missing $signature"};$b=$s.IndexOf('{',$a);$depth=1;$end=$b+1;while($depth){if($s[$end] -eq '{'){$depth++};if($s[$end] -eq '}'){$depth--};$end++};$s.Substring($a,$end-$a)}
$f=Get-Content "$out/UnworthyFixture.inc" -Raw
$f=$f.Replace('int CastSpell(Unit* target,uint32 id,bool){','virtual void AddDelayedEvent(uint64,std::function<void()>){}void InterruptNonMeleeSpells(bool){}virtual int CastSpell(Unit* target,uint32 id,bool){')
$method=Extract $f 'int CastSpell(Player*,uint32 a,bool triggered)'
$f=$f.Replace($method,'int CastSpell(Player*,uint32,bool);int CastSpell(Unit* target,uint32 id,bool triggered)override{return CastSpell(target->ToPlayer(),id,triggered);}')
$f=$f.Replace('struct SmartAI {','struct SmartAI {virtual void sGossipHello(Player*){++delegated;}')
$extra=Get-Content "$PSScriptRoot/StormheimNewBeginningPlayer.inc" -Raw
$f=$f.Replace('struct Player:Unit {',"struct Player:Unit {`n$extra`n")
[IO.File]::WriteAllText("$out/UnworthyFixture.inc",$f)
$s=Get-Content "$repo/src/server/scripts/Legion/stormheim_new_beginning.cpp" -Raw;$a=$s.IndexOf('namespace StormheimNewBeginning');$b=$s.IndexOf('void AddSC_stormheim_new_beginning()')
[IO.File]::WriteAllText("$out/NewBeginningProduction.inc",$s.Substring($a,$b-$a))
$s=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
[IO.File]::WriteAllText("$out/NativeMovieEffect.inc",(Extract $s 'void Spell::EffectPlayMovie(').Replace('Spell::','NativeMovieSpell::'))
$s=Get-Content "$repo/src/server/game/Handlers/MiscHandler.cpp" -Raw
[IO.File]::WriteAllText("$out/NativeMovieComplete.inc",(Extract $s 'void WorldSession::HandleCompleteMovie('))
$s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw;$a=$s.IndexOf('        case SMART_ACTION_INVOKER_CAST:');$b=$s.IndexOf('        case SMART_ACTION_',$a+20)
[IO.File]::WriteAllText("$out/NativeDeferredCast.inc",$s.Substring($a,$b-$a))
$s=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$near=$s.IndexOf('    if (GetMapId() == mapid && !(options & TELE_TO_ZONE_MAP))');$a=$s.IndexOf('        SetDelayedTeleportFlag(IsCanDelayTeleport());',$near);$b=$s.IndexOf('        EndMovieMovement();',$a)
[IO.File]::WriteAllText("$out/NativeNearDelay.inc",$s.Substring($a,$b-$a))
$a=$s.IndexOf('    if (IsHasDelayedTeleport() && !IsCanDelayTeleport()');$b=$s.IndexOf('    // Garrison update',$a)
[IO.File]::WriteAllText("$out/NativeDelayedUpdate.inc",$s.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimNewBeginningTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'New Beginning native regression failed'}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe";$text=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 foreach($marker in @('npc_stormheim_new_beginning_genn','Stormheim: New Beginning movie before camp transfer')){if(!$text.Contains($marker)){throw "Canonical Release lacks $marker"}}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj";$r=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 $relative=$r.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'scripts\\Release\\scripts.lib$'} | Select-Object -First 1
 if(!$relative){throw 'Missing actual scripts final-link input'}
 $lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
 if((Get-Item "$repo/src/server/scripts/Legion/stormheim_new_beginning.cpp").LastWriteTimeUtc -gt (Get-Item $lib).LastWriteTimeUtc -or (Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw 'Canonical Release predates New Beginning implementation'}
 'PASS: canonical Release registration/diagnostic and actual final-link library freshness.'
}
