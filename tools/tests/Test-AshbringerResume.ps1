param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/ashbringer-resume/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$s,[string]$name){$a=$s.IndexOf($name);if($a -lt 0){throw "Missing $name"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $path='src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/broken_shore.cpp';$source=[IO.File]::ReadAllText("$repo/$path")
 if($Baseline){$source=(& git -C $repo show "2e6f0d739:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned resume source missing'}}
 $actor=Block $source 'class spell_bs_dominate_mind :';$a=$actor.IndexOf('bool unbrekable = false;');$b=$actor.IndexOf('void Register()',$a)
 [IO.File]::WriteAllText("$out/Aura.inc",$actor.Substring($a,$b-$a).Replace('AuraEffectHandleModes','uint8'))
 $header=[IO.File]::ReadAllText("$repo/src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/broken_shore.h")
 [IO.File]::WriteAllText("$out/Constants.inc",(Block $header 'enum Data')+';'+(Block $header 'enum eCreatures')+';'+(Block ([IO.File]::ReadAllText("$repo/src/server/game/Miscellaneous/SharedDefines.h")) 'enum SpellCastResult')+';')
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $define=if($Baseline){'/DASHBRINGER_RESUME_BASELINE'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $define /I`"$out`" `"$PSScriptRoot/AshbringerResumeTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd" *> "$out/test.log";if($LASTEXITCODE){Get-Content "$out/test.log" -Tail 15;throw 'Native resume callback model failed'}
 Get-Content "$out/test.log"|Where-Object {$_ -match '^(PASS|COUNTERFACTUAL)'}
}
