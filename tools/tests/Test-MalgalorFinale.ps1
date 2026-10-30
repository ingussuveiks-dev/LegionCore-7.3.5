param([switch]$AddressSanitizer,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/malgalor-finale"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){if($s[$b] -eq '{'){++$depth};if($s[$b] -eq '}'){--$depth};++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($out,$repo,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $path='src/server/scripts/Scenario/Artifacts/Warrior/Intro/wars_intro_scenario.cpp';$s=[IO.File]::ReadAllText("$repo/$path")
 Copy-Item "$repo/src/server/scripts/Scenario/Artifacts/Warrior/Intro/malgalor_finale.h" "$out/Finale.inc"
 Copy-Item "$repo/src/server/scripts/Scenario/Artifacts/Warrior/Intro/malgalor_scene.h" "$out/Scene.inc"
 Copy-Item "$repo/src/server/scripts/Scenario/Artifacts/Warrior/Intro/reinforcement_transition.h" "$out/Reinforcement.inc"
 [IO.File]::WriteAllText("$out/Actor.inc",(Block $s 'class npc_malgalor :')+';')
 $info=[Diagnostics.ProcessStartInfo]::new('git',"show b0f7888a6:$path");$info.WorkingDirectory=$repo;$info.UseShellExecute=$false;$info.CreateNoWindow=$true;$info.RedirectStandardOutput=$true;$info.StandardOutputEncoding=[Text.Encoding]::UTF8
 $git=[Diagnostics.Process]::Start($info);$old=$git.StandardOutput.ReadToEnd();$git.WaitForExit();if($git.ExitCode){throw 'Pinned Malgalor baseline absent'};$git.Dispose()
 [IO.File]::WriteAllText("$out/Baseline.inc",(Block $old 'class npc_malgalor :')+';')
 foreach($actor in @('class npc_ragnvald_dragonborn :','class npc_danica_the_reclaimer :')){if((Block $s $actor).Replace("`r",'') -cne (Block $old $actor).Replace("`r",'')){throw "Unrelated Warrior intro actor changed: $actor"}}
 $oldBez=Block $old 'class npc_bezzered :';$newBez=Block $s 'class npc_bezzered :'
 [IO.File]::WriteAllText("$out/LegacyBezzeredes.inc",(Block $oldBez 'void JustDied('))
 $oldBez=$oldBez.Replace((Block $oldBez 'void JustDied('),'')
 function Without-Comments([string]$text){[regex]::Replace([regex]::Replace($text,'//[^\r\n]*',''),'\s+','')}
 if((Without-Comments $oldBez) -cne (Without-Comments $newBez)){throw 'Bezzeredes changed beyond moving its death transition to instance ownership'}
 $s=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/instance_broken_shore.cpp" -Raw
 if(([regex]::Matches($s,'void CreatureDies\(')).Count -ne 1 -or !(Block $s 'void CreatureDies(').Contains('MalgalorDeath(creature, killer);')){throw 'Shared native death hook must call the Warrior fallback exactly once'}
 $info.Arguments='show b0f7888a6:src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/instance_broken_shore.cpp'
 $git=[Diagnostics.Process]::Start($info);$oldInstance=$git.StandardOutput.ReadToEnd();$git.WaitForExit();if($git.ExitCode){throw 'Pinned shared instance baseline absent'};$git.Dispose()
 if(([regex]::Matches((Block $s 'void CreatureDies('),'warriorTransition.Death\(creature, killer\);')).Count -ne 1){throw 'Missing unique reinforcement instance death hook'}
 $current=(Block $s 'void CreatureDies(').Replace('Unit* killer','Unit* /*killer*/').Replace("            MalgalorDeath(creature, killer);`r`n",'').Replace("            MalgalorDeath(creature, killer);`n",'').Replace("            warriorTransition.Death(creature, killer);`r`n",'').Replace("            warriorTransition.Death(creature, killer);`n",'').Replace("`r",'')
 if($current -cne (Block $oldInstance 'void CreatureDies(').Replace("`r",'')){throw 'Original Paladin death dispatch changed'}
 [IO.File]::WriteAllText("$out/InstanceDeath.inc",(Block $s 'void MalgalorDeath(').Replace('void MalgalorDeath(','void Instance::CreatureDies('))
 $s=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
 [IO.File]::WriteAllText("$out/QuestDelay.inc",(Block $s 'void Player::AddQuestDelayedEvent('))
 [IO.File]::WriteAllText("$out/LoginExit.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/PlayerScenario.cpp" -Raw) 'bool Player::ExitCompletedBrokenShoreLogin('))
 $s=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw;$a=$s.IndexOf('    damage = uint32(std::min<double>(damage / victim->GetHealthMultiplierForTarget(this), std::numeric_limits<uint32>::max()));');$b=$s.IndexOf('    // A player charmed by an NPC',$a)
 if($a -lt 0 -or $b -le $a){throw 'Native final-scaling callback path missing'}
 [IO.File]::WriteAllText("$out/PostScale.inc",$s.Substring($a,$b-$a))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/MalgalorFinaleTest.cpp`" /Fe:test.exe /Fo:test.obj /Fd:test.pdb`nif errorlevel 1 exit /b 1`ntest.exe`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Malgalor native regression failed'}
 $prior=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile MalgalorCatalog.cpp -ProbeName MalgalorCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log"}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
 $native=Get-Content "$out/native.tsv" -Raw
 foreach($expected in @("CRITERIA`t45773`t92`t46880","CRITERIA`t45777`t92`t46881","STEP`t933`t3`t45772","STEP`t933`t4`t45776")){if(!$native.Contains($expected)){throw "Reinforcement native transition changed: $expected"}}
 if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$priorPython=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$priorPython";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_warrior_reinforcement_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Reinforcement linked PE failed'}}finally{$env:PYTHONPATH=$priorPython}}
}
