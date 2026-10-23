param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/dalaran-class-intro",[string]$OffersFile="",[string]$PairsFile="")
$ErrorActionPreference='Stop'
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$root="$PSScriptRoot/../.."
$source=Get-Content "$root/src/server/game/Spells/SpellMgr.cpp" -Raw
$a=$source.IndexOf('bool SpellArea::IsFitToRequirements(')
$b=$source.IndexOf('// Extra conditions',$a)
if($a -lt 0 -or $b -lt 0){throw 'SpellArea common requirements not found'}
# This quest has no hardcoded spell-specific case: exercise the common predicate.
[IO.File]::WriteAllText("$output/ClassIntroArea.inc",$source.Substring($a,$b-$a)+"`n    }`n    return true;`n}`n")
$source=Get-Content "$root/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$source.IndexOf('void Player::SetQuestUpdate(');$b=$source.IndexOf('void Player::SendMusic(',$a)
if($a -lt 0 -or $b -lt 0){throw 'Quest aura lifecycle handler not found'}
[IO.File]::WriteAllText("$output/ClassIntroQuestUpdate.inc",$source.Substring($a,$b-$a))
$source=Get-Content "$root/src/server/game/Quests/QuestDef.h" -Raw
$a=$source.IndexOf('enum QuestStatus');$b=$source.IndexOf('};',$a)+2
[IO.File]::WriteAllText("$output/ClassIntroStatus.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found'}
$extra=if($OffersFile -and $PairsFile){'"'+[IO.Path]::GetFullPath($OffersFile)+'" "'+[IO.Path]::GetFullPath($PairsFile)+'"'}else{''}
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/DalaranClassIntroTest.cpp" /Fe"$output/DalaranClassIntroTest.exe" /Fo"$output/DalaranClassIntroTest.obj"
if errorlevel 1 exit /b 1
"$output/DalaranClassIntroTest.exe" $extra
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Dalaran class introduction production regression failed'}
