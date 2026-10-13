param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/new-blood-cleanup")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
function Export-Section($file,$start,$end,$name) {
 $source=Get-Content "$repo/$file" -Raw
 $a=$source.IndexOf($start); $b=$source.IndexOf($end,$a)
 if($a -lt 0 -or $b -lt 0){throw "Missing production section: $start"}
 [IO.File]::WriteAllText("$output/$name",$source.Substring($a,$b-$a))
}
Export-Section 'src/server/game/Entities/Object/QuestObjectScope.h' 'struct QuestObjectScope' '#endif' 'Scope.inc'
Export-Section 'src/server/game/Entities/Object/Object.cpp' 'void WorldObject::BindQuestObjectScope(' 'GameObject* WorldObject::SummonGameObject(' 'ObjectScope.inc'
Export-Section 'src/server/game/Entities/Player/Player.cpp' 'void Player::BindQuestObject(' 'void Player::RemoveActiveQuest(' 'PlayerScope.inc'
$map=Get-Content "$repo/src/server/game/Maps/Map.cpp" -Raw
$a=$map.IndexOf('if (!summon->InheritQuestObjectScope(summoner))')
if($a -lt 0 -or $map.IndexOf('summon->InitSummon();',$a) -lt $a){throw 'Scope must precede synchronous summon AI'}
$creature=Get-Content "$repo/src/server/game/Entities/Creature/TemporarySummon.cpp" -Raw
if($creature -notmatch '(?s)void TempSummon::Update\(uint32 diff\)\s*\{[^}]*IsQuestObjectExpired\(\)[^}]*UnSummon\(\);[^}]*return;\s*}\s*Creature::Update'){throw 'Expired summon can execute AI before cleanup'}
$handler=Get-Content "$repo/src/server/game/Handlers/QuestHandler.cpp" -Raw
if($handler -notmatch 'RemoveActiveQuest\(questId\);\s+if \(questId == 40729\)\s+player->ResetNewBloodQuest\(\);'){throw 'Abandon hook missing or ordered before quest removal'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/NewBloodQuestCleanupTest.cpp" /Fe"$output/NewBloodQuestCleanupTest.exe" /Fo"$output/NewBloodQuestCleanupTest.obj"
if errorlevel 1 exit /b 1
"$output/NewBloodQuestCleanupTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'New Blood quest cleanup regression failed'}
