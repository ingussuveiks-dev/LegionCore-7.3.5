param([string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$d=1;while($d){$d+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/healing-width-callbacks"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw;$creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
$heal=Block $unit 'uint64 Unit::DealHeal(';$last=$heal.IndexOf('    Unit* unit = this;');if($last -lt 0){throw 'Native prefix boundary missing'}
[IO.File]::WriteAllText("$out/DealHeal.inc",$heal.Substring(0,$last).Replace(', SpellInfo const* spellProto','')+"    return gain;`n}")
[IO.File]::WriteAllText("$out/AddHealth.inc",(Block $unit 'uint64 Unit::AddHealth('))
[IO.File]::WriteAllText("$out/Notify.inc",(Block $creature 'void Creature::NotifyHealApplied(')+"`n"+(Block $creature 'void Creature::NotifyHealApplied64('))
[IO.File]::WriteAllText("$out/Default.inc",(Block (Get-Content "$repo/src/server/game/AI/CreatureAI.h" -Raw) 'virtual void OnHealApplied('))
[IO.File]::WriteAllText("$out/Destroy.inc",(Block $creature 'bool Creature::AIM_Destroy('))
$init=Block $creature 'bool Creature::AIM_Initialize(';[IO.File]::WriteAllText("$out/Initialize.inc",$init.Substring(0,$init.IndexOf('    AIM_Destroy();'))+"    (void)ai;`n    return true;`n}")
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /O2 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/HealingWidthCallbacksTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Full-width native prefix/notification ASAN failed'}
