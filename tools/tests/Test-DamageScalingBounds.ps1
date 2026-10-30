param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$OutputDirectory='')
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot certify Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$path='src/server/game/Entities/Unit/Unit.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'')
$pinned=(& git -C $repo show "a148a34b5:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned native division baseline missing'}
$old='    damage /= victim->GetHealthMultiplierForTarget(this);'
$current='    damage = uint32(std::min<double>(damage / victim->GetHealthMultiplierForTarget(this), std::numeric_limits<uint32>::max()));'
$comment="    // Float division can round UINT32_MAX to 2^32, or exceed it when scaling`n    // down. Clamp before the integer conversion; retain ordinary float rounding.`n"
if($source.TrimEnd("`n") -cne $pinned.Replace($old,$comment+$current).TrimEnd("`n")){throw 'Native Unit source changed beyond numeric bound'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/damage-scaling-bounds"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
[IO.File]::WriteAllText("$out/Scaling.inc",$(if($Baseline){$old}else{$current}))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /O2 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/DamageScalingBoundsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw "Damage scaling numeric regression failed: $LASTEXITCODE"}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_damage_scaling_bounds_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical damage scaling bound missing'}}finally{$env:PYTHONPATH=$previous}
}
