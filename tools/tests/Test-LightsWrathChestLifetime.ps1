param([switch]$AddressSanitizer,[switch]$Restoration)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/lights-wrath-chest-lifetime";$common="$repo/build-extractors/runtime-test/gameobject-summon-insertion"
New-Item -ItemType Directory -Force $out|Out-Null
function Section([string]$s,[string]$start,[string]$end){$a=$s.IndexOf($start);$b=$s.IndexOf($end,[Math]::Max(0,$a));if($a -lt 0 -or $b -le $a){throw "Native section missing $start"};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo,$out,$common) -Test {
 & "$PSScriptRoot/Test-GameObjectSummonInsertion.ps1" *> "$out/insertion-preservation.log";if($LASTEXITCODE){throw 'Native insertion preservation failed'}
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 [IO.File]::WriteAllText("$out/SummonGo.inc",(Section $s '        case SMART_ACTION_SUMMON_GO:' '        case SMART_ACTION_KILL_UNIT:'))
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
 [IO.File]::WriteAllText("$out/SummonFlags.inc",(Section $s 'enum SmartSummonGameObjectFlags' 'struct SmartAction'))
 $s=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
 [IO.File]::WriteAllText("$out/RemoveAllGameObjects.inc",(Section $s 'void Unit::RemoveAllGameObjects()' 'void Unit::SendSpellNonMeleeDamageLog('))
 $s=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
 [IO.File]::WriteAllText("$out/IsSpawned.inc",(Section $s 'bool GameObject::isSpawned() const' 'bool GameObject::isSpawnedByDefault() const'))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 $fixtureMode=if($Restoration){'restoration'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" /I`"$common`" `"$PSScriptRoot/LightsWrathChestLifetimeTest.cpp`" /Fe:test.exe /Fo:test.obj`nif errorlevel 1 exit /b 1`ntest.exe $fixtureMode`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native chest lifetime regression failed'}
}
