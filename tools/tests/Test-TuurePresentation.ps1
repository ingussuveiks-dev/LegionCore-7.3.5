param([switch]$Baseline,[switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/tuure-presentation"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/$(if($Baseline){'baseline'}else{'current'})" -RuntimeDirectories @($out) -Test {
 $path='src/server/scripts/Scenario/Artifacts/Priest/Tuure/Tuure.cpp'
 $source=Get-Content "$repo/$path" -Raw
 $old=(& git -C $repo show "d9137d5a:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Tuure baseline unavailable'}
 if($Baseline -and $old.Contains('scene_tuure_pickup')){ $legacy=(& git -C $repo show "44e7df34:$path") -join "`n"; if($legacy.Contains('scene_tuure_pickup')){throw 'Historical unbound baseline unavailable'} }
 # Encounter and native pickup now have separate production-handler regressions.
 foreach($name in @('namespace TuurePresentation')){if(((Block $source $name)-replace '\s','') -cne ((Block $old $name)-replace '\s','')){throw "Unrelated Tuure handler changed: $name"}}
 [IO.File]::WriteAllText("$out/Presentation.inc",(Block $source 'namespace TuurePresentation')+(Block $source 'class scene_tuure_pickup :')+';')
 $fixture=(Get-Content "$PSScriptRoot/SceneLifecycleFixture.h" -Raw).Replace('128826','128825').Replace('200262','210734')
 $pending=Block (Get-Content "$repo/src/server/game/Entities/Player/Player.h" -Raw) 'bool HasPendingTeleport() const'
 $fixture=$fixture.Replace('    bool IsBeingTeleported() const { return teleporting; }',"    bool IsBeingTeleported() const { return teleporting; }`n    bool m_bHasDelayedTeleport=false;`n    $pending")
 [IO.File]::WriteAllText("$out/TuureFixture.h",$fixture)
 $player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw;$a=$player.IndexOf('void Player::SendSpellScene(');$b=$player.IndexOf('void Player::HandleArenaDeserter()',$a)
 [IO.File]::WriteAllText("$out/Lifecycle.inc",$player.Substring($a,$b-$a))
 $handlers=Get-Content "$repo/src/server/game/Handlers/SceneHandler.cpp" -Raw
 [IO.File]::WriteAllText("$out/Handlers.inc",$handlers.Substring($handlers.IndexOf('void WorldSession::HandleScenePlaybackCanceled(')))
 [IO.File]::WriteAllText("$out/ForceEquip.inc",(Block (Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw) 'void Spell::EffectForceEquipItem('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $def=if($Baseline){'/DBASELINE'}else{''};if($AddressSanitizer){$def+=' /fsanitize=address /Zi'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $def /I`"$out`" `"$PSScriptRoot/TuurePresentationTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Tuure scene/equip regression failed'}
}
