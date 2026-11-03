param([string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/althea-death/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"};$out=[IO.Path]::GetFullPath($OutputDirectory)
function Block([string]$source,[string]$start){$a=$source.IndexOf($start);if($a -lt 0){throw 'Missing actual actor'};$b=$source.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($source[$b] -eq '{'))-([int]($source[$b] -eq '}'));++$b};$source.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 & "$PSScriptRoot/Test-KingslayersDeathCredit.ps1" -AddressSanitizer
 if(!$?){throw 'Retained actual Merlis/native owner export failed'}
 foreach($file in @('MerlisDeath.inc','OwnerGuid.inc','OwnerPlayer.inc','PlayerFind.inc','PlayerLookup.inc','PlayerLookupMethod.inc','ScenarioGate.inc')){Copy-Item "$repo/build-extractors/runtime-test/kingslayers-death-credit/$file" "$out/$file"}
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp';$source=Get-Content "$repo/$path" -Raw;$bound=Block $source 'class AltheaDeathConversation final : public Conversation';[IO.File]::WriteAllText("$out/AltheaActor.inc",$bound+';');$source=$source.Replace($bound+';','');$old=(& git -C $repo show "21ace0503:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Althea source missing'}
 # Door activation has its own complete source/native regression. Normalize
 # only that isolated class before retaining the earlier whole-file certificate.
 $door=Block $source 'class go_kingslayers_door : public GameObjectScript';$oldDoor=Block $old 'class go_kingslayers_door : public GameObjectScript';$source=$source.Replace($door,$oldDoor).Replace('#include "ScriptsData.h"','')
 $current=Block (Block $source 'struct npc_sister_althea_ebonlockeAI : ScriptedAI') 'void JustDied(Unit* who) override'
 $victory=Block (Block $source 'struct npc_sister_althea_ebonlockeAI : ScriptedAI') 'Player* GetVictoryPlayer('
 [IO.File]::WriteAllText("$out/AltheaVictory.inc",$victory);$source=$source.Replace($victory,'')
 $before=Block (Block $old 'struct npc_sister_althea_ebonlockeAI : ScriptedAI') 'void JustDied(Unit* who) override'
 if((($source.Replace($current,$before) -replace '//[^\n]*','') -replace '\s','') -cne (($old -replace '//[^\n]*','') -replace '\s','')){throw 'Only Althea death may change; all other AI/quests/actors retained'}
 [IO.File]::WriteAllText("$out/Althea.inc",$current);[IO.File]::WriteAllText("$out/AltheaBefore.inc",$before)
 $unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw;$a=$unit.IndexOf('    // Prevent killing unit twice',$unit.IndexOf('void Unit::Kill('));$b=$unit.IndexOf('    volatile uint32',$a)
 [IO.File]::WriteAllText("$out/AltheaKillGuard.inc",$unit.Substring($a,$b-$a))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/AltheaDeathTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Althea actual death ASAN failed'}
 $command=Get-Content "$out/run.cmd" -Raw
 $command=$command.Replace('AltheaDeathTest.cpp','AltheaActorResolverTest.cpp').Replace('test.exe','resolver.exe').Replace('test.obj','resolver.obj').Replace('test.pdb','resolver.pdb')
 [IO.File]::WriteAllText("$out/resolver.cmd",$command)
 & "$out/resolver.cmd";if($LASTEXITCODE){throw 'Actual full Althea actor resolver ASAN failed'}
}
