$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/sharasdal-ascent-counterfactual"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Test-SharasdalErunakUpper.ps1" -PrepareHost -OutputDirectory $out
if(!$?){throw 'Baseline native actor/quest/function/lifecycle host failed'}
$source=(& git -C $repo show "066dddfa6:src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp") -join "`n";if($LASTEXITCODE){throw 'Pinned ascent counterfactual missing'}
[IO.File]::WriteAllText("$out/Actor.inc",(Block $source 'class npc_erunak_stonespeaker :')+';')
$upperHost=Get-Content "$PSScriptRoot/SharasdalErunakUpperTest.cpp" -Raw
[IO.File]::WriteAllText("$out/ReplyHost.inc",$upperHost.Substring(0,$upperHost.IndexOf('int main(){')))
$hostSource=Get-Content "$out/TimerHost.inc" -Raw
$a='void CastSpell(Unit*,uint32){}';$b='std::vector<uint32> ascentCasts;void CastSpell(Unit* target,uint32 id){assert(target==this&&id==208580);ascentCasts.push_back(id);}'
if(!$hostSource.Contains($a)){throw 'Player cast boundary changed'};$hostSource=$hostSource.Replace($a,$b)
$a='Player* FindNearestPlayer(float){return nullptr;}';if(!$hostSource.Contains($a)){throw 'Grid visitor boundary changed'};$hostSource=$hostSource.Replace($a,'Player* nearest=nullptr;Player* FindNearestPlayer(float){return nearest;}')
$hostSource=$hostSource.Replace('virtual void MovementInform(uint32,uint32){}','virtual void LastWPReached(){}virtual void MovementInform(uint32,uint32){}')
[IO.File]::WriteAllText("$out/TimerHost.inc",$hostSource)
$native=Get-Content "$repo/src/server/game/Movement/MovementGenerators/WaypointMovementGenerator.cpp" -Raw
$body=(Block $native 'void WaypointMovementGenerator<Creature>::MovementInform(').Replace('void WaypointMovementGenerator<Creature>::MovementInform(','void NativeWaypoint<T>::MovementInform(')
[IO.File]::WriteAllText("$out/NativeWaypointDispatch.inc",'template<class T> '+$body)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/SharasdalAscentCounterfactual.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Actual ascent counterfactual failed'}
