param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot certify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/silver-hand-procession"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "8c95a0ba:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned procession baseline unavailable'}}
$actor=Block $source 'struct npc_travar_secondAI : ScriptedAI'
[IO.File]::WriteAllText("$out/ProcessionPolicy.inc",(Block $source 'namespace SilverHandProcession'))
[IO.File]::WriteAllText("$out/ProcessionGossip.inc",(Block $actor 'void sGossipSelect(').Replace(' override',''))
[IO.File]::WriteAllText("$out/ProcessionArrival.inc",$(if($Baseline){''}else{Block $actor 'void CompleteProcession('}))
[IO.File]::WriteAllText("$out/ProcessionMovement.inc",(Block $actor 'void MovementInform(').Replace(' override',''))
$guid=Get-Content "$repo/src/server/game/Entities/Object/ObjectGuid.cpp" -Raw
[IO.File]::WriteAllText("$out/GuidLifecycle.inc",((@('ObjectGuid::ObjectGuid()','void ObjectGuid::Clear()','bool ObjectGuid::IsEmpty() const')|ForEach-Object{Block $guid $_}) -join "`n"))
$header=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.h" -Raw
[IO.File]::WriteAllText("$out/WorldPlayer.inc",(Block $header 'static Player* GetObjectInWorld(ObjectGuid'))
[IO.File]::WriteAllText("$out/MapPlayer.inc",(Block $header 'template<class T> static T* GetObjectInMap('))
[IO.File]::WriteAllText("$out/AccessorPlayer.inc",(Block (Get-Content "$repo/src/server/game/Globals/ObjectAccessor.cpp" -Raw) 'Player* ObjectAccessor::GetPlayer('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/SilverHandProcessionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native procession recipient regression failed'}
if($Baseline){return}
& "$PSScriptRoot/Test-SilverHandGossip.ps1" -Second -VerifyReleaseBinary:$VerifyReleaseBinary
if(!$?){throw 'Native procession start/data checks failed'}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_procession_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed procession completion guard missing'}}finally{$env:PYTHONPATH=$previous}
}
