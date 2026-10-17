$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-StormheimMistsCatalog.ps1"
& "$PSScriptRoot/Test-StormheimDilemma.ps1" *> "$repo/.codex/mists-upstream.log"
$out="$repo/build-extractors/runtime-test/stormheim-mists"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-dilemma/*.inc" | Copy-Item -Destination $out
$f=(Get-Content "$out/DilemmaFixture.inc" -Raw).Replace('GetEntry()==93603&&q==39153','GetEntry()==97973&&q==40044')
[IO.File]::WriteAllText("$out/MistsFixture.inc",$f)
function Export-Mists([string]$source,[string]$start,[string]$end,[string]$name){$a=$source.IndexOf($start);$b=$source.IndexOf($end,$a);if($a -lt 0 -or $b -le $a){throw "Missing native $name"};[IO.File]::WriteAllText("$out/$name.inc",$source.Substring($a,$b-$a))}
$s=Get-Content "$repo/src/server/game/AI/CreatureAISelector.cpp" -Raw
Export-Mists $s '    CreatureAI* selectAI(' '    MovementGenerator* SelectMovementGenerator(' 'MistsSelectAI'
$s=Get-Content "$repo/src/server/game/AI/CoreAI/CombatAI.cpp" -Raw
Export-Mists $s 'int AggressorAI::Permissible(' 'void AggressorAI::InitializeAI()' 'MistsPermit'
Export-Mists $s 'void AggressorAI::Reset()' 'void AggressorAI::UpdateAI(' 'MistsCombatReset'
Export-Mists $s 'void AggressorAI::EnterCombat(' 'void AggressorAI::MoveInLineOfSight(' 'MistsCombatEnter'
Export-Mists $s 'void AggressorAI::JustDied(' 'void AggressorAI::EnterCombat(' 'MistsCombatDied'
$a=$s.IndexOf('    if (uint32 spellId = spellCasts.ExecuteEvent())');$b=$s.IndexOf('void AggressorAI::DoActionAI(',$a)
$part=$s.Substring($a,$b-$a);$part=$part.Substring(0,$part.LastIndexOf('}'))
[IO.File]::WriteAllText("$out/MistsCombatDispatch.inc",$part)
$s=Get-Content "$repo/src/server/game/AI/CoreAI/ReactorAI.cpp" -Raw
Export-Mists $s 'int ReactorAI::Permissible(' 'void ReactorAI::MoveInLineOfSight(' 'MistsReactorPermit'
$s=Get-Content "$repo/src/server/game/AI/CoreAI/UnitAI.cpp" -Raw
Export-Mists $s '        if (spellInfo->HasAttribute(SPELL_ATTR0_CANT_USED_IN_COMBAT))' '        if (AIInfo->cooldown < spellInfo->Cooldowns.RecoveryTime)' 'MistsAICondition'
$s=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
Export-Mists $s 'bool CreatureSpell::CanUseInDifficulty(' 'CreatureMovementData::CreatureMovementData(' 'MistsDifficulty'
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($name in @('StormheimMistsTest','StormheimMistsCombatTest')){
 $cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/$name.cpp`" /Fe`"$out/$name.exe`" /Fo`"$out/$name.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/$name.exe`" `"$out/world.tsv`" `"$out/native.tsv`"`r`n"
 [IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
 if($LASTEXITCODE){throw "Native Mists regression failed: $name"}
}
