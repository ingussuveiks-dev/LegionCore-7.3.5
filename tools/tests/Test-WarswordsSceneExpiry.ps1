param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-scene-expiry/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$s,[string]$name){$a=$s.IndexOf($name);if($a -lt 0){throw "Missing $name"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
function Source([string]$path){if($Baseline -and ($path -like '*Player.cpp' -or $path -like '*SpellAuraEffects.cpp')){$s=(& git -C $repo show "cb9cc208e:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned finite-scene baseline missing'};return $s};[IO.File]::ReadAllText("$repo/$path")}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $s=Source 'src/server/scripts/Scenario/Artifacts/Warrior/Warswords/Warswords.cpp';$a=$s.IndexOf('namespace WarswordsPresentation');$b=$s.IndexOf('void AddSC_Warswords()',$a)
 [IO.File]::WriteAllText("$out/Presentation.inc",$s.Substring($a,$b-$a))
 $s=Source 'src/server/game/Entities/Player/Player.cpp';$a=$s.IndexOf('void Player::SendSpellScene(');$b=$s.IndexOf('void Player::HandleArenaDeserter()',$a)
 [IO.File]::WriteAllText("$out/Lifecycle.inc",$s.Substring($a,$b-$a))
 $s=Source 'src/server/game/Handlers/SceneHandler.cpp';[IO.File]::WriteAllText("$out/Handlers.inc",$s.Substring($s.IndexOf('void WorldSession::HandleScenePlaybackCanceled(')))
 $s=Source 'src/server/game/Spells/SpellEffects.cpp';[IO.File]::WriteAllText("$out/ForceEquip.inc",(Block $s 'void Spell::EffectForceEquipItem('))
 $s=Source 'src/server/game/Spells/Auras/SpellAuraEffects.cpp';[IO.File]::WriteAllText("$out/AuraScene.inc",(Block $s 'void AuraEffect::HandleAuraActivateScene('))
 $s=Source 'src/server/game/Entities/Unit/Unit.h';[IO.File]::WriteAllText("$out/AuraRemoveModes.inc",(Block $s 'enum AuraRemoveMode')+';')
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $define=if($Baseline){'/DSCENE_EXPIRY_BASELINE'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $define /I`"$out`" `"$PSScriptRoot/WarswordsSceneExpiryTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd" *> "$out/test.log";if($LASTEXITCODE){Get-Content "$out/test.log" -Tail 20;throw 'Finite scene lifecycle ASAN failed'}
 Get-Content "$out/test.log"|Where-Object {$_ -match '^(PASS|COUNTERFACTUAL)'}
}
