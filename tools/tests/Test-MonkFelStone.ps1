param([switch]$ScopeOnly,[switch]$ExportOnly,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/monk-fel-stone"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence/$(Get-Date -Format yyyyMMdd-HHmmss-fff)" -RuntimeDirectories @($repo,$out) -Test {
 function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
 function Shape([string]$s){(($s -replace '(?s)/\*.*?\*/','') -replace '//[^\n]*','') -replace '\s',''}
 $path='src/server/scripts/Scenario/scenario_script_loader.cpp';$source=Get-Content "$repo/$path" -Raw;$prior=(& git -C $repo show "53953490f:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Fel Stone predecessor missing'}
 if((Shape $source.Replace('void AddSC_monk_fel_stone();','').Replace('AddSC_monk_fel_stone();','')) -cne (Shape $prior)){throw 'Other scenario registration changed'}
 foreach($path in @('src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_intro_scenario.cpp','src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/instance_class_art_scenarios.cpp')){
  $prior=(& git -C $repo show "53953490f:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned original actor/instance missing'}
  if((Shape (Get-Content "$repo/$path" -Raw)) -cne (Shape $prior)){throw 'Original actors or shared instance changed'}
 }
 $cpp=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_fel_stone.cpp" -Raw
 $expected='void AddSC_monk_fel_stone(){RegisterCreatureAI(npc_monk_fel_stone);RegisterSpellAndAuraScriptPair(spell_monk_fel_stone,aura_monk_fel_stone_scene);RegisterSpellScript(spell_monk_fel_stone_event);RegisterSpellAndAuraScriptPair(spell_monk_fel_stone_pilgrimage,aura_monk_fel_stone_pilgrimage);new player_monk_fel_stone();}'
 if((Shape (Block $cpp 'void AddSC_monk_fel_stone()')) -cne (Shape $expected)){throw 'Exact Fel Stone factory registrations changed'}
 if($ScopeOnly){'PASS separate Fel Stone five registrations only; full previous Monk actors/shared instances and all other scenario registrations unchanged';return}
 # Access specifiers alone are opened for controlled hook calls. Handler
 # bodies, branches, native IDs, registry and cleanup compile without edits.
 [IO.File]::WriteAllText("$out/FelStone.inc",(Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Monk/Intro/monk_fel_stone.h" -Raw).Replace('private:','public:'))
 $fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
 [IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
 $timer=Get-Content "$repo/src/common/Utilities/Timer.h" -Raw
 [IO.File]::WriteAllText("$out/Timer.inc",(Block $timer 'inline uint32 getMSTimeDiff(')+(Block $timer 'inline uint32 GetMSTimeDiffToNow('))
 $unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw
 $aura=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuraDefines.h" -Raw
 [IO.File]::WriteAllText("$out/NativeAuraEnums.inc",(Block $unit 'enum AuraRemoveMode')+';'+(Block $aura 'enum AuraEffectHandleModes')+';'+(Block $unit 'enum TriggerCastFlags')+';'+(Block $unit 'enum CastType')+';')
 $sources=@{
  'NativeClick'=@('Entities/Unit/Unit.cpp','bool Unit::HandleSpellClick(')
  'NativeTrigger'=@('Spells/SpellEffects.cpp','void Spell::EffectTriggerSpell(')
  'NativeCredit'=@('Spells/SpellEffects.cpp','void Spell::EffectKillCredit(')
  'NativeEvent'=@('Spells/SpellEffects.cpp','void Spell::EffectSendEvent(')
  'NativeGroup'=@('Entities/Player/Player.cpp','void Player::RewardPlayerAndGroupAtEvent(')
  'NativeDistance'=@('Entities/Player/Player.cpp','bool Player::IsAtGroupRewardDistance(')
  'NativeScene'=@('Entities/Player/Player.cpp','void Player::SendSpellScene(')
  'NativeSceneComplete'=@('Entities/Player/Player.cpp','void Player::SceneCompleted(')
  'NativeAuraScene'=@('Spells/Auras/SpellAuraEffects.cpp','void AuraEffect::HandleAuraActivateScene(')
  'NativeCanCriteria'=@('Scenarios/Scenario.cpp','bool Scenario::CanUpdateCriteria(')
 }
 foreach($key in $sources.Keys){[IO.File]::WriteAllText("$out/$key.inc",(Block (Get-Content "$repo/src/server/game/$($sources[$key][0])" -Raw) $sources[$key][1]))}
 if($ExportOnly){return}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/MonkFelStoneTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Actual Fel Stone ASAN regression failed'}
}
