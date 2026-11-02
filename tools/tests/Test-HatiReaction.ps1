param([switch]$VerifyNativeCatalog,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/hati-reaction/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($repo,$out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
 function Shape([string]$s){(($s -replace '(?s)/\*.*?\*/','') -replace '//[^\n]*','') -replace '\s',''}
 $path='src/server/scripts/Spells/spell_hunter.cpp';$source=Get-Content "$repo/$path" -Raw;$old=(& git -C $repo show "4ea7b499e:$path") -join "`n";if($LASTEXITCODE){throw 'Missing pinned Hati baseline'}
 $actor=Block $source 'class spell_hun_hatis_bond :';$currentActor=$actor;$prior=Block $old 'class spell_hun_hatis_bond :'
 if($actor.Contains('if (!caster || !caster->IsAlive() || !caster->ToPlayer())')){
  & "$PSScriptRoot/Test-HatiOwner.ps1" -ScopeOnly
  $source=(& git -C $repo show "ee6b845e4:$path") -join "`n";if($LASTEXITCODE){throw 'Certified pre-owner source missing'};$actor=Block $source 'class spell_hun_hatis_bond :'
 }
 $normalized=$actor.Replace('hati->SetReactState(pet->GetReactState());','if (ReactStates reactState = pet->GetReactState()) hati->SetReactState(reactState);')
 if((Shape $normalized) -cne (Shape $prior) -or (Shape $source.Replace($actor,$prior)) -cne (Shape $old)){throw 'Only zero-valued reaction filter may change; complete apply/remove/timer/summon/cleanup/other scripts retained'}
 foreach($pair in @(@('Bond','BondFixed',$currentActor),@('Baseline','BondBaseline',$prior))){
  $class="struct $($pair[1]):AuraScript{uint32 update=0;"+(Block $pair[2] 'void OnUpdate(')+(Block $pair[2] 'void Register()').Replace('spell_hun_hatis_bond_AuraScript::',"$($pair[1])::")+'};'
  [IO.File]::WriteAllText("$out/$($pair[0]).inc",$class)
 }
 $creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
 $unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw
 [IO.File]::WriteAllText("$out/ReactEnums.inc",(Block $unit 'enum ReactStates')+';'+(Block $unit 'enum CommandStates')+';')
 [IO.File]::WriteAllText("$out/React.inc",(Block $creature 'void Creature::SetReactState(')+(Block $creature 'ReactStates Creature::GetReactState('))
 $pet=Get-Content "$repo/src/server/game/AI/CoreAI/PetAI.cpp" -Raw
 [IO.File]::WriteAllText("$out/Consumers.inc",(Block $pet 'void PetAI::OwnerAttacked(')+(Block $pet 'void PetAI::OwnerDamagedBy(')+(Block $pet 'bool PetAI::CanAttack('))
 $script=Get-Content "$repo/src/server/game/Spells/SpellScript.cpp" -Raw
 [IO.File]::WriteAllText("$out/Caster.inc",(Block $script 'Unit* AuraScript::GetCaster() const'))
 $header=Get-Content "$repo/src/server/game/Spells/SpellScript.h" -Raw
 $a=$header.IndexOf('enum SpellScriptState');$b=$header.IndexOf('// helper class from which SpellScript',$a);$c=$header.IndexOf('enum AuraScriptHookType');$d=$header.IndexOf('class TC_GAME_API AuraScript',$c)
 [IO.File]::WriteAllText("$out/Hooks.inc",$header.Substring($a,$b-$a)+$header.Substring($c,$d-$c))
 [IO.File]::WriteAllText("$out/Dispatch.inc",(Block (Get-Content "$repo/src/server/game/Spells/Auras/SpellAuras.cpp" -Raw) 'void Aura::CallScriptEffectUpdateHandlers('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/HatiReactionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 $info=New-Object Diagnostics.ProcessStartInfo;$info.FileName=$env:ComSpec;$info.Arguments='/d /c ""'+$out+'/build.cmd""';$info.WorkingDirectory=$out
 $info.UseShellExecute=$false;$info.CreateNoWindow=$true;$info.RedirectStandardOutput=$true;$info.RedirectStandardError=$true
 $owned=New-Object Diagnostics.Process;$owned.StartInfo=$info;if(!$owned.Start()){throw 'Owned Hati fixture launch failed'}
 $ownedPid=$owned.Id;$stdout=$owned.StandardOutput.ReadToEndAsync();$stderr=$owned.StandardError.ReadToEndAsync()
 if(!$owned.WaitForExit(60000)){throw "Owned fixture $ownedPid requires investigation"}
 $exit=$owned.ExitCode;[IO.File]::WriteAllText("$out/stdout.log",$stdout.Result);[IO.File]::WriteAllText("$out/stderr.log",$stderr.Result)
 [IO.File]::WriteAllText("$out/process.json",(@{PID=$ownedPid;Exit=$exit}|ConvertTo-Json));$owned.Dispose();$stdout.Result;$stderr.Result
 if($exit -or $stderr.Result.Length){throw "Hati native reaction regression failed exit$exit"}
 if($VerifyNativeCatalog){
  $previous=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile HatiReactionCatalog.cpp -ProbeName HatiReactionCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Hati native catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
  $native=Get-Content "$out/native.tsv"
  foreach($expected in @("BINDING`t197344`tspell_hun_hatis_bond","BOND_EFFECT`t0`t6`t4`t1","PROPERTIES`t3725`t1`t1`t0`t134744578","COMPLETE`tHatiReaction")){if($native -notcontains $expected){throw "Missing native fact $expected"}}
  foreach($entry in @(100324,106548,106549,106550,106551)){if($native -notcontains "ACTOR`t$entry`t`t"){throw 'Hati native actor binding changed'}}
  'PASS native26972/read-only hotfix+World:197344 dummy/self/binding and five original3725 Hati summons/actor bindings'
 }
 if($VerifyReleaseBinary){
  if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$previous=$env:PYTHONPATH
  try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$repo/tools/tests;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_hati_reaction_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Hati reaction Release check failed'}}finally{$env:PYTHONPATH=$previous}
 }
}
