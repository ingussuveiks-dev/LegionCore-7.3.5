param(
 [switch]$VerifyReleaseBinary,
 [string]$ReleaseDirectory="$PSScriptRoot/../../build-extractors/bin/Release"
)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/boost-ship-arrival"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$source=[IO.File]::ReadAllText("$repo/src/server/scripts/Scenario/BoostExperience/boost_experience.cpp")
foreach($method in @('PutPlayerOnTransport','OnPlayerEnter')){
 $start=$source.IndexOf("void $method(");if($start -lt 0){throw "Missing production $method"}
 $end=$source.IndexOf('{',$start)+1;$depth=1
 while($depth -and $end -lt $source.Length){if($source[$end] -eq '{'){++$depth};if($source[$end] -eq '}'){--$depth};++$end}
 [IO.File]::WriteAllText("$out/$method.inc",$source.Substring($start,$end-$start).Replace(' override',''))
}
$setup=$source.Substring($source.IndexOf('bool Setup()'),$source.IndexOf('void OnGameObjectCreate(')-$source.IndexOf('bool Setup()'))
if($setup -notmatch 'if \(player->GetTransport\(\) != transport\)\s*PutPlayerOnTransport\(player, transport\);'){throw 'Setup must not clear the pre-create arrival hold with a second teleport'}
$guard=$source.IndexOf('player->ShouldDeferMapObjectArrivalVisibility(passenger)')
$force=$source.IndexOf('passenger->SendUpdateToPlayer(player);')
if($guard -lt 0 -or $guard -gt $force -or $source.Substring($guard,$force-$guard) -notmatch '_visibilityTimer = 100;\s*continue;'){throw 'Forced passenger visibility bypasses the shared arrival hold'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/BoostShipArrivalTest.cpp`" /Fe`"$out/arrival.exe`" /Fo`"$out/arrival.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/arrival.exe`"`r`n"
[IO.File]::WriteAllText("$out/arrival.cmd",$cmd);& "$out/arrival.cmd"
if($LASTEXITCODE){throw 'Boost ship arrival regression failed'}
if($VerifyReleaseBinary){
 $python=(Get-Command python -ErrorAction SilentlyContinue | Select-Object -First 1).Source
 if(!$python){$python='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'}
 & $python "$PSScriptRoot/verify_boost_ship_arrival_release.py" $ReleaseDirectory
 if($LASTEXITCODE){throw 'Linked Release boost ship arrival verification failed'}
}
