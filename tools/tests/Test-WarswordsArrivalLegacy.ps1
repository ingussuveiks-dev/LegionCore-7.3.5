param([switch]$Baseline,[switch]$LoadingBaseline)
$ErrorActionPreference='Stop'

$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-arrival"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Warrior/Warswords/instance_warswords.cpp'
$source=(& git -C $repo show "85469a518:$path") -join "`n"
if($Baseline){$source=(& git -C $repo show "9fe2be7b:$path") -join "`n";if($LASTEXITCODE){throw 'Missing pinned arrival baseline'}}
foreach($name in @('Initialize','OnPlayerEnter','Update')){[IO.File]::WriteAllText("$out/$name.inc",(Block $source "void $name("))}
[IO.File]::WriteAllText("$out/Policy.inc",$(if($Baseline){''}else{Block $source 'namespace WarswordsArrival'}))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}elseif($LoadingBaseline){'/DLOADING_BASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/WarswordsArrivalTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Actual Warswords arrival callback regression failed'}
