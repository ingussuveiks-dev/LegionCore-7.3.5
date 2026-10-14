$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-impalement"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_impalement.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$output/Impalement.inc",$source.Substring(0,$source.IndexOf('void AddSC_stormheim_impalement()')))
$core=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
$a=$core.IndexOf('SpellCastResult Spell::CanOpenLock(');$b=$core.IndexOf('void Spell::SetSpellValue(',$a)
[IO.File]::WriteAllText("$output/CanOpenLock.inc",$core.Substring($a,$b-$a))
$effects=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
$method=$effects.IndexOf('void Spell::EffectOpenLock(');$a=$effects.IndexOf('    SkillType skillId = SKILL_NONE;',$method);$b=$effects.IndexOf('    // not allow use skill grow',$a)
[IO.File]::WriteAllText("$output/OpenLockCompletion.inc",$effects.Substring($a,$b-$a))
$objects=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
$a=$objects.IndexOf('void GameObject::Use(Unit* user)');$b=$objects.IndexOf('    // If cooldown data present',$a)
[IO.File]::WriteAllText("$output/UsePrefix.inc",$objects.Substring($a,$b-$a))
$conditions=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$a=$conditions.IndexOf('        case CONDITION_QUESTTAKEN:');$b=$conditions.IndexOf('        case CONDITION_QUEST_COMPLETE:',$a)
[IO.File]::WriteAllText("$output/QuestTaken.inc",$conditions.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$output`" `"$PSScriptRoot/StormheimImpalementTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$output/test.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Impalement native completion regression failed'}
