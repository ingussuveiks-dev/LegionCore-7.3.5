param([switch]$Baseline)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/dynamic-model-nearest/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force "$out/logs"|Out-Null
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$previous=$env:LEGION_DIALOGUE_CONFIG;$compiler=$env:CL;$link=$env:_LINK_
$sealed=@(Get-FileHash -LiteralPath @("$PSScriptRoot/DynamicModelNearestNative.cpp","$repo/src/common/collision/DynamicTree.cpp","$repo/src/common/collision/RegularGrid.h","$repo/src/common/collision/BoundingIntervalHierarchyWrapper.h","$repo/build-extractors/src/common/Release/common.lib","$repo/build-extractors/src/server/game/Release/game.lib","$repo/build-extractors/src/server/scripts/Release/scripts.lib")|Select-Object Path,Hash)
$sealed|ConvertTo-Json|Set-Content "$out/inputs-before.json"
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf"
 $env:CL="$compiler /DNDEBUG /Zi /I`"$out`" /I`"$repo/src/server/scripts/Legion`" /I`"$repo/src/server/scripts/Legion/MawofSouls`""
 if($Baseline){$env:CL+=' /DDYNAMIC_MODEL_NEAREST_BASELINE';$original=(& git -C $repo show be9af5a45:src/common/collision/DynamicTree.cpp) -join "`n";if(!$original){throw 'Pinned baseline source missing'};[IO.File]::WriteAllText("$out/DynamicTreeActual.inc",$original)}
 $env:_LINK_="$link /DEBUG /PDB:`"$out/native.pdb`" /MAP:`"$out/native.map`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{
  & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
   & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/DynamicModelNearestNative.cpp -ProbeName DynamicModelNearestNative -OutputFile "$out/native.tsv" *> "$out/compiler.log"
   if(!$? -or $LASTEXITCODE){throw "Native closest-surface check failed:$out/compiler.log"}
   Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(PATH|PASS)'}
  }
 }finally{Pop-Location}
}finally{
 $env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$compiler;$env:_LINK_=$link
 foreach($input in $sealed){if((Get-FileHash -LiteralPath $input.Path).Hash -ne $input.Hash){throw 'Native closest-surface input changed during verification'}}
}
