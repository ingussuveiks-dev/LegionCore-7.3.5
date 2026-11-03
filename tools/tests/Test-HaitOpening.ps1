param([switch]$ScopeOnly,[switch]$ExportOnly,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
. "$PSScriptRoot/HaitDefeatPredecessor.ps1"
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/hait-opening/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"};$out=[IO.Path]::GetFullPath($OutputDirectory)
$evidence=if($ExportOnly){"$out/opening-export-evidence"}else{"$out/evidence"}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $evidence -RuntimeDirectories @($repo,$out) -Test {
 function Shape([string]$s){(($s -replace '(?s)/\*.*?\*/','') -replace '//[^\n]*','') -replace '\s',''}
 function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw 'Unclosed source'};$s.Substring($a,$b-$a)}
 function Prior([string]$path){$s=(& git -C $repo show "c0f7fae1a:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned complete pre-opening source missing'};$s}
 $path='src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_intro_scenario.cpp';$actualSource=Get-Content "$repo/$path" -Raw;$source=Get-HaitPreDefeatSource $path $actualSource;$oldSource=Prior $path
 $normal=$source.Replace('#include "hait_opening.h"','').Replace('if (MonkHaitOpening::Source(me)) return;','')
 if((Shape $normal) -cne (Shape $oldSource)){throw 'Only initial Hait action1 scoped exclusion/include permitted; complete other actors retained'}
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/instance_class_art_scenarios.cpp';$instance=Get-HaitPreDefeatSource $path (Get-Content "$repo/$path" -Raw);$oldInstance=Prior $path
 $normal=$instance.Replace((Block $instance 'void OnCreatureDamageTaken('),'').Replace('#include "../../Monk/Intro/hait_opening.h"','').Replace(', haitOpening(new MonkHaitOpening::State(map))','').Replace('std::unique_ptr<MonkHaitOpening::State> haitOpening;','').Replace(' haitOpening->Observe(actor);','').Replace('if (type == MonkHaitOpening::AcceptedData) return haitOpening->owner;','').Replace('if (type == MonkHaitOpening::AcceptData) haitOpening->Accept(value);','').Replace('haitOpening->Update(diff); ','')
 if((Shape $normal) -cne (Shape $oldInstance)){throw 'Only appended owned opening/actual create-GUID-update bridge may change; entire shared instance retained'}
 $path='src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_hait_click.h';$click=Get-Content "$repo/$path" -Raw;$oldClick=Prior $path
 $normal=$click.Replace((Block $click 'if (auto script = source->GetInstanceScript())'),'')
 $a=$normal.IndexOf('auto script = player.GetMap()->GetCreature(claim->source)->GetInstanceScript();');$b=$normal.IndexOf('player.GetMap()->GetCreature(claim->source)->RemoveFlag',$a);if($a -lt 0 -or $b -lt 0){throw 'Accepted instance transfer missing'}
 $normal=$normal.Remove($a,$b-$a).Replace('player.GetMap()->GetCreature(claim->source)->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);','player.GetMap()->GetCreature(claim->source)->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK); actor->DoAction(1);')
 if((Shape $normal) -cne (Shape $oldClick)){throw 'Only after-receipt instance transfer/accepted replay guard may change; native click/effects/receipt/retry retained'}
 if($ScopeOnly){'PASS exact initial Hait scoped action exclusion, after-real-receipt transfer/replay guard and appended instance owner; full old actors/click/native/other shared instances retained';return}
 & "$PSScriptRoot/Test-MonkChenEarlyPresentation.ps1" -ExportOnly -OutputDirectory $out
 if(!$?){throw 'Current native actor/instance host export failed'}
 [IO.File]::WriteAllText("$out/HaitOpeningActor.inc",(Block $actualSource 'class npc_master_hait :')+';')
 [IO.File]::WriteAllText("$out/HaitOpeningBeforeActor.inc",(Block $oldSource 'class npc_master_hait :').Replace('npc_master_hait','npc_master_hait_before_opening')+';')
 [IO.File]::WriteAllText("$out/HaitOpeningBeforeInstance.inc",(Block $oldInstance 'struct instance_monk_intro :').Replace('instance_monk_intro','instance_monk_intro_before_opening')+';')
 $unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw;$at=$unit.IndexOf('instance->OnCreatureDamageTaken(creature, this, damage);');$from=$unit.LastIndexOf('if (Creature* creature = victim->ToCreature())',$at)
 [IO.File]::WriteAllText("$out/HaitDamageDispatch.inc",(Block $unit.Substring($from) 'if (Creature* creature = victim->ToCreature())'))
 if($ExportOnly){return}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/HaitOpeningTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";$code=$LASTEXITCODE;[IO.File]::WriteAllText("$out/native-result.json",(@{Exit=$code}|ConvertTo-Json));if($code){throw 'Actual opening/controller/native dispatch ASAN failed'}
 if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$prior=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$prior";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_hait_opening_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Registered opening Release proof failed'}}finally{$env:PYTHONPATH=$prior}}
}
