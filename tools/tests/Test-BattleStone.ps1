param([switch]$AddressSanitizer)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/battle-stone"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){if($s[$b] -eq '{'){++$depth};if($s[$b] -eq '}'){--$depth};++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($out,$repo) -Test {
 $s=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
 foreach($pair in @(@('Quality','static uint8 BattlePetUpgradeQuality('),@('Check','SpellCastResult Spell::CheckBattlePetUpgrade('),@('Effect','void Spell::EffectUpgradeBattlePet('))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(Block $s $pair[1]))}
 $baseline=(& git -C $repo show 'b981afb7b:src/server/game/Spells/SpellEffects.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned stone baseline missing'}
 [IO.File]::WriteAllText("$out/Baseline.inc",(Block $baseline 'void Spell::EffectUpgradeBattlePet('))
 $s=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
 [IO.File]::WriteAllText("$out/Consume.inc",(Block $s 'void Player::DestroyItemCount(Item*'))
 $s=Get-Content "$repo/src/server/game/PetBattle/PetBattle.cpp" -Raw
 [IO.File]::WriteAllText("$out/Stats.inc",(Block $s 'void BattlePet::UpdateStats('))
 $s=Get-Content "$repo/src/server/game/Handlers/BattlePetHandler.cpp" -Raw
 [IO.File]::WriteAllText("$out/Notify.inc",(Block $s 'void WorldSession::HandleBattlePetUpdateDisplayNotify('))
 $s=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
 [IO.File]::WriteAllText("$out/TakeCastItem.inc",(Block $s '    if (expendable && withoutCharges)'))
 $a=$s.IndexOf('            case SPELL_EFFECT_UPGRADE_BATTLE_PET:',$s.IndexOf('SpellCastResult Spell::CheckCast('));$b=$s.IndexOf('            case SPELL_EFFECT_TRIGGER_SPELL:',$a)
 if($a -lt 0 -or $b -le $a -or $s.Substring($a,$b-$a) -notmatch 'CheckBattlePetUpgrade'){throw 'Native prelaunch upgrade check missing'}
 $s=Get-Content "$repo/src/server/game/Server/Protocol/Opcodes.cpp" -Raw
 if($s -notmatch 'CMSG_BATTLE_PET_UPDATE_DISPLAY_NOTIFY,\s+STATUS_LOGGEDIN,\s+PROCESS_THREADUNSAFE,\s+&WorldSession::HandleBattlePetUpdateDisplayNotify'){throw 'Display handler not registered on world thread'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/BattleStoneTest.cpp`" /Fe:test.exe /Fo:test.obj`nif errorlevel 1 exit /b 1`ntest.exe`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Battle-stone native regression failed'}
}
