param([switch]$Baseline,[switch]$HealBaseline,[switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
if($Baseline -and $HealBaseline){throw 'Select one counterfactual'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/tuure-ally-damage"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/$(if($Baseline){'baseline'}elseif($HealBaseline){'heal-baseline'}else{'current'})" -RuntimeDirectories @($repo,$out) -Test {
 $path='src/server/scripts/Scenario/Artifacts/Priest/Tuure/Tuure.cpp';$source=Get-Content "$repo/$path" -Raw
 $old=(& git -C $repo show "7608b3f2:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Tuure ally baseline unavailable'}
 [IO.File]::WriteAllText("$out/Recovery.inc",$(if($Baseline -or $HealBaseline){''}else{Block $source 'namespace TuureCompanion'}))
 foreach($pair in @(@('Boros','npc_vindicator_borosAI'),@('Allies','npc_tuure_small_eventsAI'))){
  $actor=Block $source "struct $($pair[1]) : ScriptedAI";$pinned=Block $old "struct $($pair[1]) : ScriptedAI"
  [IO.File]::WriteAllText("$out/$($pair[0])Damage.inc",$(if($Baseline){Block $pinned 'void DamageTaken('}else{Block $actor 'void DamageTakenAfterScaling('}))
  [IO.File]::WriteAllText("$out/$($pair[0])Heal.inc",(Block $(if($Baseline -or $HealBaseline){$pinned}else{$actor}) 'void HealReceived('))
  [IO.File]::WriteAllText("$out/$($pair[0])Lifecycle.inc",$(if($Baseline -or $HealBaseline){''}else{Block $actor 'void UpdateLifecycle('}))
  [IO.File]::WriteAllText("$out/$($pair[0])Reset.inc",(Block $(if($Baseline -or $HealBaseline){$pinned}else{$actor}) 'void Reset('))
 }
 # Reuse the established native-health harness and parameterized boundary/race
 # cases. These are test adapters, never donor production implementation.
 $fixture=Get-Content "$PSScriptRoot/SilverHandAllyDamageTest.cpp" -Raw
 $nativeHost=$fixture.Substring(0,$fixture.IndexOf('struct EventMap {')).Replace('210985','204237')
 $nativeHost=$nativeHost.Replace('struct Player {','struct Player {void UpdateAchievementCriteria(uint32,uint32){};')
 $nativeHost=$nativeHost.Replace('    CreatureAI* GetAI();','    Player* ToPlayer(){return nullptr;} CreatureAI* GetAI();')
 $nativeHost=$nativeHost.Replace('    bool HasScalableLevels() const',"    Creature* GetMotionMaster(){return this;}void MovePoint(uint32,float,float,float){}void SetByteValue(uint32,uint32,uint32){}void SetAnimKitId(uint32){}`n    bool HasScalableLevels() const")
 $nativeHost=$nativeHost.Replace('enum { UNIT_STATE_STUNNED=8,','enum { UNIT_FIELD_BYTES_1=1, UNIT_STATE_STUNNED=8,')
 [IO.File]::WriteAllText("$out/NativeHost.inc",$nativeHost)
 $a=$fixture.IndexOf('uint32 Unit::Deal(');$b=$fixture.IndexOf('int main()', $a)
 [IO.File]::WriteAllText("$out/NativeCases.inc",$fixture.Substring($a,$b-$a).Replace('SILVER_HAND','TUURE_ALLY'))
 $core=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
 $heal=Block $core 'int32 Unit::DealHeal(';$last=$heal.IndexOf('    Unit* unit = this;');if($last -lt 0){throw 'Native DealHeal boundary missing'}
 [IO.File]::WriteAllText("$out/DealHealPrefix.inc",$heal.Substring(0,$last).Replace(', SpellInfo const* spellProto','')+"    return gain;`n}")
 foreach($pair in @(@('ModifyHealth','int64 Unit::ModifyHealth('),@('Health','uint64 Unit::GetHealth(Unit* victim) const'))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $core $pair[1]))}
 $a=$core.IndexOf('    damage /= victim->GetHealthMultiplierForTarget(this);');if($a -lt 0){throw 'Native damage scaling missing'}
 [IO.File]::WriteAllText("$out/ScaledHook.inc",'damage /= victim->GetHealthMultiplierForTarget(this);'+(Block $core.Substring($a) '    if (Creature* creature = victim->ToCreature())'))
 $creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
 [IO.File]::WriteAllText("$out/Multiplier.inc",(Block $creature 'float Creature::GetHealthMultiplierForTarget('))
 [IO.File]::WriteAllText("$out/LifecycleBridge.inc",(Block $creature 'if (i_AI || i_disabledAI)'))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $def=if($Baseline){'/DTUURE_ALLY_ALLY_BASELINE'}elseif($HealBaseline){'/DTUURE_ALLY_HEAL_BASELINE'}else{''};if($AddressSanitizer){$def+=' /fsanitize=address /Zi'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $def /I`"$out`" `"$PSScriptRoot/TuureAllyDamageTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Tuure ally native regression failed'}
}
