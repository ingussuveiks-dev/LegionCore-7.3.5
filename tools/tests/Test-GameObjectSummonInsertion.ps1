param([switch]$Baseline,[switch]$MapOnly,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot certify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/gameobject-summon-insertion"
New-Item -ItemType Directory -Force $out | Out-Null
function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing section $start"};$source.Substring($first,$last-$first)}
function Source([string]$path){if($Baseline){$result=(& git -C $repo show "f9b38107c3b36be1f4421d709f4be739d623da31:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-fix source missing'};return $result};Get-Content "$repo/$path" -Raw}
$source=Source 'src/server/game/Entities/Object/Object.cpp'
[IO.File]::WriteAllText("$out/WorldSummon.inc",(Section $source 'GameObject* WorldObject::SummonGameObject(uint32 entry, Position' 'Creature* WorldObject::SummonTrigger('))
[IO.File]::WriteAllText("$out/WorldCleanup.inc",(Section $source 'void WorldObject::CleanupsBeforeDelete(' 'uint32 WorldObject::GetZoneId()'))
$source=Source 'src/server/game/Maps/Map.cpp'
[IO.File]::WriteAllText("$out/MapSummon.inc",(Section $source 'GameObject* Map::SummonGameObject(uint32 entry, Position' 'void Map::SummonCreatureGroup('))
$source=Source 'src/server/game/Entities/Unit/Unit.cpp'
[IO.File]::WriteAllText("$out/Ownership.inc",(Section $source 'void Unit::AddGameObject(' 'void Unit::RemoveGameObject(uint32'))
$source=Source 'src/server/game/Entities/GameObject/GameObject.cpp'
[IO.File]::WriteAllText("$out/GameObjectCleanup.inc",(Section $source 'void GameObject::CleanupsBeforeDelete(' 'void GameObject::AddToWorld()'))
$source=Source 'src/server/game/Entities/Transport/Transport.cpp'
[IO.File]::WriteAllText("$out/TransportCleanup.inc",(Section $source 'void Transport::CleanupsBeforeDelete(' 'void Transport::Update('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$arguments=if($MapOnly){'map'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/GameObjectSummonInsertionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" $arguments`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native gameobject insertion contract failed'}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_gameobject_summon_insertion_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Deployed insertion contract failed'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: WorldObject and Map summon APIs preserve native owner-before-insertion and cleanup on false map admission, then return null; map-grid failure is controlled in this regression, not an observed live artifact failure.'
