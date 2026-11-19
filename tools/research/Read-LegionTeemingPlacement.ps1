param([Parameter(Mandatory=$true)][string]$InputFile,[string]$ModelDataDirectory)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$inputPath=(Resolve-Path -LiteralPath $InputFile).Path
$out="$repo/build-extractors/runtime-test/legion-teeming-placement/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force "$out/logs"|Out-Null
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$previous=$env:LEGION_DIALOGUE_CONFIG;$points=$env:LEGION_TEEMING_POINTS;$compiler=$env:CL;$link=$env:_LINK_
$models=$env:LEGION_MAW_MODEL_DATA
$sealed=@(Get-FileHash -LiteralPath @($inputPath,"$PSScriptRoot/LegionTeemingPlacementNative.cpp","$repo/build-extractors/src/common/Release/common.lib","$repo/build-extractors/src/server/game/Release/game.lib","$repo/build-extractors/src/server/scripts/Release/scripts.lib")|Select-Object Path,Hash)
if($ModelDataDirectory){$sealed+=@(Get-ChildItem -LiteralPath "$ModelDataDirectory/mmaps" -Filter 'go*.mmap' -File|Get-FileHash|Select-Object Path,Hash)}
$sealed|ConvertTo-Json|Set-Content "$out/inputs-before.json"
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:LEGION_TEEMING_POINTS=$inputPath
 $env:LEGION_MAW_MODEL_DATA=if($ModelDataDirectory){(Resolve-Path -LiteralPath $ModelDataDirectory).Path.Replace('\','/')}else{$null}
 $env:CL="$compiler /DNDEBUG /Zi /I`"$repo/src/server/scripts/Legion`" /I`"$repo/src/server/scripts/Legion/MawofSouls`""
 $env:_LINK_="$link /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{
  & "$repo/tools/tests/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
   & "$PSScriptRoot/Read-LegionLegendaryCatalog.ps1" -SourceFile LegionTeemingPlacementNative.cpp -ProbeName LegionTeemingPlacementNative -OutputFile "$out/surfaces.tsv" *> "$out/compiler.log"
   if(!$? -or $LASTEXITCODE){throw "Native physical survey failed:$out/compiler.log"}
   Get-Content "$out/surfaces.tsv"|Where-Object {$_ -match '^PASS'}
  }
 }finally{Pop-Location}
}finally{
 $env:LEGION_DIALOGUE_CONFIG=$previous;$env:LEGION_TEEMING_POINTS=$points;$env:CL=$compiler;$env:_LINK_=$link
 $env:LEGION_MAW_MODEL_DATA=$models
 if(Test-Path -LiteralPath "$out/worldserver.conf"){Remove-Item -LiteralPath "$out/worldserver.conf" -Force}
 foreach($item in $sealed){if((Get-FileHash -LiteralPath $item.Path).Hash -ne $item.Hash){throw 'Placement input changed'}}
}
"SURVEY_OUTPUT $out/surfaces.tsv"
