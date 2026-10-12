param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/highmountain-the-three")
$ErrorActionPreference='Stop'
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$root="$PSScriptRoot/../.."
$source=Get-Content "$root/src/server/game/Spells/SpellEffects.cpp" -Raw
$a=$source.IndexOf('bool Spell::SpellDummyTriggered(');$b=$source.IndexOf('void Spell::EffectDummy(',$a)
$function=$source.Substring($a,$b-$a)
$start=$function.IndexOf('            switch (itr.option)')
$caseStart=$function.IndexOf('                case DUMMY_TRIGGER_CAST_OR_REMOVE:')
$caseEnd=$function.IndexOf('                case DUMMY_TRIGGER_DAM_MAXHEALTH:',$caseStart)
$tail=$function.IndexOf('            if(itr.group != 0 && check)',$caseEnd)
if($a -lt 0 -or $b -lt 0 -or $start -lt 0 -or $caseStart -lt 0 -or $tail -lt 0){throw 'Missing production dummy trigger sections'}
$extracted=$function.Substring(0,$start)+"            switch (itr.option) {`n"+$function.Substring($caseStart,$caseEnd-$caseStart)+"            }`n"+$function.Substring($tail)
[IO.File]::WriteAllText("$output/ThreeDummy.inc",$extracted)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found'}
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/HighmountainTheThreeTest.cpp" /Fe"$output/HighmountainTheThreeTest.exe" /Fo"$output/HighmountainTheThreeTest.obj"
if errorlevel 1 exit /b 1
"$output/HighmountainTheThreeTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'The Three production regression failed'}
