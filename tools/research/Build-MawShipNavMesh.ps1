param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/maw-ship-model/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force "$out/logs",$OutputDirectory|Out-Null
$destination=(Resolve-Path -LiteralPath $OutputDirectory).Path
if($destination.StartsWith("$repo/build-extractors/bin/Release",[StringComparison]::OrdinalIgnoreCase)){throw 'Generate into staging; canonical deployment is separate'}
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$previous=$env:LEGION_DIALOGUE_CONFIG;$modelOut=$env:LEGION_MAW_MODEL_OUTPUT;$compiler=$env:CL;$link=$env:_LINK_
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:LEGION_MAW_MODEL_OUTPUT=$destination
 $env:CL="$compiler /DNDEBUG /Zi /I`"$repo/src/server/scripts/Legion`" /I`"$repo/src/server/scripts/Legion/MawofSouls`" /I`"$repo/dep/recastnavigation/Recast/Include`""
 $env:_LINK_="$link /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`" `"$repo/build-extractors/dep/recastnavigation/Recast/Release/Recast.lib`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{
  & "$repo/tools/tests/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
   & "$PSScriptRoot/Read-LegionLegendaryCatalog.ps1" -SourceFile BuildMawShipNavMesh.cpp -ProbeName BuildMawShipNavMesh -OutputFile "$out/models.tsv" *> "$out/compiler.log"
   if(!$? -or $LASTEXITCODE){throw "Model generation failed:$out/compiler.log"}
   Get-Content "$out/models.tsv"|Where-Object {$_ -match '^(INPUT|RASTER|POLYGONS|MODEL|STATES|PASS)'}
  }
 }finally{Pop-Location}
}finally{
 $env:LEGION_DIALOGUE_CONFIG=$previous;$env:LEGION_MAW_MODEL_OUTPUT=$modelOut;$env:CL=$compiler;$env:_LINK_=$link
}
"MODEL_EVIDENCE $out"
