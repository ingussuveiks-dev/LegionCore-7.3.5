$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/new-blood-prisoners"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=Get-Content "$repo/src/server/scripts/Legion/new_blood.cpp" -Raw
$a=$source.IndexOf('struct npc_new_blood_prisoner');$b=$source.IndexOf('// Own the archive handoff', $a)
if($a -lt 0 -or $b -lt 0){throw 'Prisoner production AI not found'}
[IO.File]::WriteAllText("$output/NewBloodPrisoner.inc",$source.Substring($a,$b-$a))
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$method=$unit.IndexOf('bool Unit::_IsValidAssistTarget(')
$a=$unit.IndexOf('    if (!bySpell || !(bySpell->HasAttribute(SPELL_ATTR6_ASSIST_IGNORE_IMMUNE_FLAG)))',$method)
$b=$unit.IndexOf('    if (Player const* player = ToPlayer())',$a)
[IO.File]::WriteAllText("$output/RescueAssistImmunity.inc",$unit.Substring($a,$b-$a))
$a=$unit.IndexOf('    else if (HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE)',$b)
$b=$unit.IndexOf('    return true;', $a)
[IO.File]::WriteAllText("$output/RescueAssistCreature.inc",$unit.Substring($a,$b-$a).Replace('else if','if'))
$fix=Get-Content "$repo/src/server/game/Spells/SpellMgr.cpp" -Raw
$a=$fix.IndexOf('ApplySpellFix({195039}');$a=$fix.IndexOf('{',$fix.IndexOf('[](SpellInfo* spellInfo)', $a))+1;$b=$fix.IndexOf('    });',$a)
[IO.File]::WriteAllText("$output/RescueSpellFix.inc",$fix.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$output`" `"$PSScriptRoot/NewBloodPrisonerTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$output/test.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Prisoner lifecycle regression failed'}
$cmd=$cmd.Replace('NewBloodPrisonerTest.cpp','NewBloodRescueTargetTest.cpp')
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Rescue target regression failed'}
