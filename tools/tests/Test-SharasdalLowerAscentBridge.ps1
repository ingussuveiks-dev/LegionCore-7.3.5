param([string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Test-SharasdalLowerAscent.ps1" -ScopeOnly
if(!$?){throw 'Current ascent source certificate failed'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-lower-ascent-bridge"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Test-SharasdalUpperWall.ps1" -PrepareHost -OutputDirectory $out
if(!$?){throw 'Retained raw gossip/native quest host preparation failed'}
$source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp" -Raw
[IO.File]::WriteAllText("$out/Actor.inc",(Block $source 'class npc_grash :')+';')
$hostSource=Get-Content "$out/NativeHost.inc" -Raw
if(!$hostSource.Contains('struct InstanceScript{')){throw 'Instance service boundary changed'}
$hostSource=$hostSource.Replace('struct InstanceScript{','struct InstanceScript{void SetGuidData(uint32,ObjectGuid);ObjectGuid GetGuidData(uint32)const;')
$hostSource=$hostSource.Replace('bool operator==(ObjectGuid other)const{return high==other.high&&low==other.low;}','bool operator==(ObjectGuid other)const{return high==other.high&&low==other.low;}bool operator!=(ObjectGuid other)const{return !(*this==other);}')
[IO.File]::WriteAllText("$out/NativeHost.inc",$hostSource)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/SharasdalLowerAscentBridgeTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Current Grash raw gossip/installed-path bridge regression failed'}
