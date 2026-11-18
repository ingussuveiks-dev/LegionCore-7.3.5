param([Parameter(Mandatory=$true)][string]$ModelDataDirectory,[switch]$Baseline)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/maw-ship-navigation/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force "$out/logs"|Out-Null
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$previous=$env:LEGION_DIALOGUE_CONFIG;$models=$env:LEGION_MAW_MODEL_DATA;$compiler=$env:CL;$link=$env:_LINK_
$sealed=@(Get-FileHash -LiteralPath @("$PSScriptRoot/MawShipNavigationNative.cpp","$repo/src/server/game/Movement/MawShipGroundPath.h","$repo/src/server/game/Movement/PathGenerator.cpp","$repo/build-extractors/src/common/Release/common.lib","$repo/build-extractors/src/server/game/Release/game.lib","$repo/build-extractors/src/server/scripts/Release/scripts.lib")|Select-Object Path,Hash)
$sealed+=@(Get-ChildItem -LiteralPath "$ModelDataDirectory/mmaps" -Filter 'go*.mmap' -File|Get-FileHash|Select-Object Path,Hash)
$sealed|ConvertTo-Json|Set-Content "$out/inputs-before.json"
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:LEGION_MAW_MODEL_DATA=(Resolve-Path -LiteralPath $ModelDataDirectory).Path.Replace('\','/')
 $env:CL="$compiler /DNDEBUG /Zi /I`"$out`" /I`"$repo/src/server/scripts/Legion`" /I`"$repo/src/server/scripts/Legion/MawofSouls`""
 if($Baseline){$env:CL+=' /DMAW_SHIP_PATH_BASELINE';$original=(& git -C $repo show be9af5a45:src/server/game/Movement/PathGenerator.cpp) -join "`n";if(!$original){throw 'Pinned baseline source missing'};[IO.File]::WriteAllText("$out/PathGeneratorActual.inc",$original)}
 $env:_LINK_="$link /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{
  & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
   & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/MawShipNavigationNative.cpp -ProbeName MawShipNavigationNative -OutputFile "$out/native.tsv" *> "$out/compiler.log"
   if(!$? -or $LASTEXITCODE){throw "Native Maw navigation failed:$out/compiler.log"}
   Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PATH|PASS)'}
  }
 }finally{Pop-Location}
}finally{
 $env:LEGION_DIALOGUE_CONFIG=$previous;$env:LEGION_MAW_MODEL_DATA=$models;$env:CL=$compiler;$env:_LINK_=$link
 foreach($input in $sealed){if((Get-FileHash -LiteralPath $input.Path).Hash -ne $input.Hash){throw 'Native ship navigation input changed during verification'}}
}
