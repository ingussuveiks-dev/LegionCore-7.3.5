param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$OutputDirectory='')
$ErrorActionPreference='Stop'
if($VerifyReleaseBinary -and ($Baseline -or $ScopeOnly)){throw 'Only repaired healing certifies Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth -and $last -lt $source.Length){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};if($depth){throw "Unclosed $start"};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'')
$newGrash=$source.Contains('namespace SharasdalGrash')
$newRescue=$source.Contains('namespace SharasdalRescue')
if(!$Baseline -and $newGrash){
 & "$PSScriptRoot/Test-SharasdalGrashHealing.ps1" -ScopeOnly
 # The separate full Grash host compiles its final-heal/Reset/lifecycle hooks.
 # Keep this earlier three-role matrix as a strictly scoped wound countercheck.
 $source=(& git -C $repo show "e17413eaa:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned retained three-role healing context missing'}
}
$pinned=(& git -C $repo show "c3c3ce355:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned wound/heal baseline missing'}
$pairs=@(@('Erunak','npc_erunak_stonespeaker'),@('Grash','npc_grash'),@('Adel','npc_adel_waterspeaker'))
if(!$Baseline){
 $whole=$source
 foreach($pair in $pairs){
  $old=Block $pinned "class $($pair[1]) :";$current=Block $source "class $($pair[1]) :"
  $damage=(Block $old 'void DamageTakenAfterScaling(').Replace('                    wait_heal = true;',"                    _pendingHealRecovery = false;`n                    wait_heal = true;")
  $reset=(Block $old 'void Reset(').Replace('            events.Reset();',"            _pendingHealRecovery = false;`n            events.Reset();")
  $heal=Block $old 'void HealReceived(';$tail=Block $heal 'if (wait_heal)'
  $heal=$heal.Replace('uint32& /*addhealth*/','uint32& addhealth').Replace($tail,"if (wait_heal && addhealth)`n                _pendingHealRecovery = true;")
  if($pair[0] -ne 'Grash'){$heal=$heal.Replace('Unit* who','Unit* /*who*/')}
  foreach($expected in @(@('void DamageTakenAfterScaling(',$damage),@('void Reset(',$reset),@('void HealReceived(',$heal))){if(((Block $current $expected[0]) -replace '\s','') -cne ($expected[1] -replace '\s','')){throw 'Unrelated damage/reset/intro-heal branch changed'}}
  $life=Block $current 'void UpdateLifecycle('
  if(($life -replace '\s','') -cne 'voidUpdateLifecycle(uint32)override{SharasdalCompanion::Recover(me,_pendingHealRecovery,wait_heal);}') {throw 'Unexpected wound lifecycle behavior'}
  $normalized=$current.Replace($life,'').Replace('bool _pendingHealRecovery = false;','')
  foreach($method in @('void DamageTakenAfterScaling(','void Reset(','void HealReceived(')){$normalized=$normalized.Replace((Block $normalized $method),(Block $old $method))}
  if(($normalized -replace '\s','') -cne ($old -replace '\s','')){throw 'Unrelated companion movement/action/timer/field changed'}
  $whole=$whole.Replace($current,$old)
 }
 $whole=$whole.Replace((Block $whole 'namespace SharasdalCompanion'),'')
 if(($whole -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Unrelated Sharasdal actor, click, boss, loot or registration changed'}
}
if($ScopeOnly){if($newGrash){'PASS retained three40pct policy through frozen e17413eaa plus exact new Grash source certificate; current Grash final-heal/intro hooks checked separately'}else{'PASS exact three40pct pending-heal/reset/re-wound/lifecycle scope; Grash50/90pct/story/others unchanged'};return}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-companion-healing"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$selected=if($Baseline){$pinned}else{$source}
[IO.File]::WriteAllText("$out/Recovery.inc",$(if($Baseline){''}else{Block $source 'namespace SharasdalCompanion'}))
foreach($pair in $pairs){
 $actor=Block $selected "class $($pair[1]) :"
 foreach($method in @(@('Damage','void DamageTakenAfterScaling('),@('Reset','void Reset('))){[IO.File]::WriteAllText("$out/$($pair[0])$($method[0]).inc",(Block $actor $method[1]))}
 $heal=Block $actor 'void HealReceived('
 if($pair[0] -eq 'Grash'){$start=if($Baseline){'if (wait_heal)'}else{'if (wait_heal && addhealth)'};$tail=if($Baseline){Block $heal $start}else{$heal.Substring($heal.IndexOf($start))};$heal='void HealReceived(Unit*,uint32& addhealth) override {'+$tail+$(if($Baseline){'}'}else{''})}
 [IO.File]::WriteAllText("$out/$($pair[0])Heal.inc",$heal)
 [IO.File]::WriteAllText("$out/$($pair[0])Lifecycle.inc",$(if($Baseline){''}else{Block $actor 'void UpdateLifecycle('}))
}
$fixture=Get-Content "$PSScriptRoot/SilverHandAllyDamageTest.cpp" -Raw
[IO.File]::WriteAllText("$out/NativeHost.inc",$fixture.Substring(0,$fixture.IndexOf('struct EventMap {')).Replace('210985','208093'))
$first=$fixture.IndexOf('uint32 Unit::Deal(');$last=$fixture.IndexOf('int main()',$first)
[IO.File]::WriteAllText("$out/NativeCases.inc",$fixture.Substring($first,$last-$first).Replace('SILVER_HAND','SHARASDAL'))
$core=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$heal=Block $core 'int32 Unit::DealHeal(';$last=$heal.IndexOf('    Unit* unit = this;');if($last -lt 0){throw 'Native heal boundary missing'}
[IO.File]::WriteAllText("$out/DealHealPrefix.inc",$heal.Substring(0,$last).Replace(', SpellInfo const* spellProto','')+"    return gain;`n}")
foreach($method in @(@('ModifyHealth','int64 Unit::ModifyHealth('),@('Health','uint64 Unit::GetHealth(Unit* victim) const'))){[IO.File]::WriteAllText("$out/$($method[0]).inc",(Block $core $method[1]))}
$prefix='    damage = uint32(std::min<double>(damage / victim->GetHealthMultiplierForTarget(this), std::numeric_limits<uint32>::max()));';$first=$core.IndexOf($prefix);if($first -lt 0){throw 'Native bounded scaling missing'}
[IO.File]::WriteAllText("$out/ScaledHook.inc",$prefix+(Block $core.Substring($first) '    if (Creature* creature = victim->ToCreature())'))
$creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/Multiplier.inc",(Block $creature 'float Creature::GetHealthMultiplierForTarget('))
[IO.File]::WriteAllText("$out/LifecycleBridge.inc",(Block $creature 'if (i_AI || i_disabledAI)'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DSHARASDAL_HEAL_BASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/SharasdalCompanionHealingTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw "Sharasdal native heal regression failed: $LASTEXITCODE"}
if($newGrash -and !$Baseline){'Retained matrix uses pinned e17413eaa after exact new Grash scope; current full Grash hooks have their separate native regression'}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";$verifier=if($newRescue){'verify_sharasdal_rescue_release.py'}elseif($newGrash){'verify_sharasdal_grash_healing_release.py'}else{'verify_sharasdal_companion_healing_release.py'};& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/$verifier" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical post-heal lifecycle missing'}}finally{$env:PYTHONPATH=$previous}
}
