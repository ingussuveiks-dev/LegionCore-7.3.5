param([switch]$AddressSanitizer,[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/heroic-leap-impact"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){if($s[$b] -eq '{'){++$depth};if($s[$b] -eq '}'){--$depth};++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($out,$repo,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $s=Get-Content "$repo/src/server/game/Movement/MovementGenerators/PointMovementGenerator.h" -Raw
 [IO.File]::WriteAllText("$out/EffectGenerator.inc",(Block $s 'class EffectMovementGenerator :')+';')
 $s=Get-Content "$repo/src/server/game/Movement/MovementGenerators/PointMovementGenerator.cpp" -Raw
 [IO.File]::WriteAllText("$out/EffectMethods.inc",(Block $s 'bool EffectMovementGenerator::Update(')+"`n"+(Block $s 'void EffectMovementGenerator::Finalize(')+"`n"+(Block $s 'void EffectMovementGenerator::MovementInform('))
 $s=Get-Content "$repo/src/server/game/Movement/MotionMaster.cpp" -Raw
 [IO.File]::WriteAllText("$out/MotionMethods.inc",(Block $s 'void MotionMaster::Mutate(')+"`n"+(Block $s 'void MotionMaster::DirectDelete('))
 [IO.File]::WriteAllText("$out/Jump.inc",(Block $s 'void MotionMaster::MoveJump(float'))
 $s=Get-Content "$repo/src/server/scripts/Spells/spell_warrior.cpp" -Raw
 [IO.File]::WriteAllText("$out/SuppressOuter.inc",(Block $s 'void HandleOuterJump('))
 $s=$s.Substring($s.IndexOf('class spell_warr_heroic_leap_jump'))
 [IO.File]::WriteAllText("$out/SuppressMissile.inc",(Block $s 'void HandleMissile('))
 $s=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
 [IO.File]::WriteAllText("$out/ChargeTimer.inc",(Block $s 'uint32 Player::GetSpellCategoryChargesTimer(SpellCategoryEntry const*'))
 [IO.File]::WriteAllText("$out/MaxCharges.inc",(Block $s 'uint8 Player::GetMaxSpellCategoryCharges(SpellCategoryEntry const*'))
 [IO.File]::WriteAllText("$out/PlayerMask.inc",(Block $s 'bool Player::IsAffectedBySpellmod('))
 [IO.File]::WriteAllText("$out/ApplyMod.inc","template<class T>`n"+(Block $s 'void Player::ApplySpellMod('))
 $mask=Get-Content "$repo/src/server/game/Spells/SpellInfo.cpp" -Raw
 [IO.File]::WriteAllText("$out/SpellMask.inc",(Block $mask 'bool SpellInfo::IsAffectedBySpellMod(SpellModifier*'))
 $s=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
 $gate=Block $s.Substring($s.IndexOf('bool Unit::HasAuraLinkedSpell(')) 'case LINK_HAS_AURA_ON_CASTER:'
 [IO.File]::WriteAllText("$out/CasterAuraGate.inc",'switch(0){'+$gate.Replace('case LINK_HAS_AURA_ON_CASTER:','case 0:')+'}')
 $ap=Block $s.Substring($s.IndexOf('float ApCoeffMod = effectInfo->BonusCoefficientFromAP;')) 'if (ApCoeffMod > 0)'
 [IO.File]::WriteAllText("$out/AttackPower.inc",$ap)
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/HeroicLeapImpactTest.cpp`" /Fe:test.exe /Fo:test.obj`nif errorlevel 1 exit /b 1`ntest.exe`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Heroic Leap landing/talent regression failed'}
 if($VerifyNativeCatalog){
  $prior=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile HeroicLeapImpactCatalog.cpp -ProbeName HeroicLeapImpactCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
  $rows=@(Get-Content "$out/native.tsv")
  foreach($row in @('TALENT 22627 spell 202163 spec 0','PVP_TALENT 166 spell 199148','PVP_TALENT 178 spell 199086','CHARGE 6544 1211 45000 1','CLASS 52174 4 0 536870912 0 0','MASK 199148 1 0 536870912 0 0','DURATION 202164 3000','DURATION 199085 3000','RADIUS 14 8 8')){if($rows -notcontains $row){throw "Native Leap contract differs: $row"}}
  foreach($pattern in @('EFFECT 52174 0 diff 0 type 2 * base 1 AP 0.624 *','EFFECT 202163 0 diff 0 type 6 aura 453 misc 1211,0 * base -15000 *','EFFECT 202164 0 diff 0 type 6 aura 31 * base 70 *','EFFECT 199148 0 diff 0 type 6 aura 411 misc 1211,0 * base 2 *','EFFECT 199148 1 diff 0 type 6 aura 108 misc 0,0 * base 200 *')){if(!@($rows|Where-Object {$_ -like $pattern}).Count){throw "Native Leap effect differs: $pattern"}}
  'PASS matching DB2/hotfix damage/radius/normal and PvP talent metadata'
 }
}
