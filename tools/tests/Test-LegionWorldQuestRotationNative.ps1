param([string]$Migration="$PSScriptRoot/../../sql/updates/world/2026_11_19_12_restore_legion_world_quest_rotation.sql")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$migrationPath=(Resolve-Path -LiteralPath $Migration).Path
$out="$repo/build-extractors/runtime-test/legion-world-quest-rotation/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force "$out/logs"|Out-Null
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$previous=$env:LEGION_DIALOGUE_CONFIG
$previousMigration=$env:LEGION_ROTATION_MIGRATION
$compiler=$env:CL
$link=$env:_LINK_
$inputs=@($migrationPath,"$PSScriptRoot/LegionWorldQuestRotationNative.cpp","$PSScriptRoot/LegionAreaDialogueNative.cpp",
 "$repo/src/server/game/Globals/QuestData.cpp","$repo/src/server/game/Entities/Player/Player.cpp","$repo/src/server/game/World/World.cpp",
 "$repo/build-extractors/src/server/game/Release/game.lib","$repo/build-extractors/src/server/scripts/Release/scripts.lib")
$sealed=@(Get-FileHash -LiteralPath $inputs|Select-Object Path,Hash)
$sealed|ConvertTo-Json|Set-Content "$out/inputs-before.json"
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf"
 $env:LEGION_ROTATION_MIGRATION=$migrationPath
 $env:CL="$compiler /DNDEBUG /Zi /I`"$repo/src/server/scripts/Legion`" /I`"$repo/src/server/scripts/Legion/MawofSouls`""
 $env:_LINK_="$link /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{
  & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
   & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/LegionWorldQuestRotationNative.cpp -ProbeName LegionWorldQuestRotationNative -OutputFile "$out/native.tsv" *> "$out/compiler.log"
   Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PASS|ROTATION)'}
  }
 }finally{Pop-Location}
}finally{
 $env:LEGION_DIALOGUE_CONFIG=$previous
 $env:LEGION_ROTATION_MIGRATION=$previousMigration
 $env:CL=$compiler
 $env:_LINK_=$link
 if(Test-Path -LiteralPath "$out/worldserver.conf"){Remove-Item -LiteralPath "$out/worldserver.conf" -Force}
 foreach($item in $sealed){if((Get-FileHash -LiteralPath $item.Path).Hash -ne $item.Hash){throw 'Rotation verification input changed'}}
}
"WORLD_QUEST_ROTATION_OUTPUT $out"
