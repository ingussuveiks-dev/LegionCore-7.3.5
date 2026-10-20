param([string]$BaselineRevision)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/override-aura-replacement"
if($BaselineRevision){$out+='-baseline'}
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Extract([string]$s,[string]$start,[string]$end,[string]$name){$a=$s.IndexOf($start);$b=$s.IndexOf($end,$a);if($a -lt 0 -or $b -lt $a){throw "Missing production section $start"};[IO.File]::WriteAllText("$out/$name.inc",$s.Substring($a,$b-$a))}
$s=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuraEffects.cpp" -Raw
Extract $s 'void AuraEffect::HandleEffect(AuraApplication * aurApp' 'void AuraEffect::HandleEffect(Unit* target' 'EffectDispatcher'
if($BaselineRevision){$s=(& git -C $repo show "${BaselineRevision}:src/server/game/Spells/Auras/SpellAuraEffects.cpp") -join "`n";if($LASTEXITCODE){throw 'Cannot read baseline override handler'}}
Extract $s 'void AuraEffect::HandleAuraOverrideSpells(' 'void AuraEffect::HandleAuraSetVehicle(' 'OverrideHandler'
$s=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
Extract $s 'void Unit::_RegisterAuraEffect(' '// All aura base removes' 'RegisterEffect'
$s=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
Extract $s 'void Player::AddTemporarySpell(' 'bool Player::IsNeedCastPassiveSpellAtLearn(' 'TemporarySpells'
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
# Checked MSVC iterators turn the original erased-list iterator into a
# deterministic regression failure instead of depending on allocator luck.
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /MDd /D_ITERATOR_DEBUG_LEVEL=2 /I`"$out`" `"$PSScriptRoot/OverrideAuraReplacementTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw "Override aura replacement regression failed (native exit $LASTEXITCODE)"}
& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile OverrideAuraCatalog.cpp -ProbeName OverrideAuraCatalog -OutputFile "$out/native.tsv"
$native=Get-Content "$out/native.tsv"
foreach($expected in @('effect 198425 0 293 641','override 641 198424')){if($native -notcontains $expected){throw "Crash spell native DB2 mismatch: $expected"}}
'PASS: native crash spell198425 is aura293/override641/button198424'
