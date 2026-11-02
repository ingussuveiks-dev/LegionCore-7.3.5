param([ValidateSet('Chen','Hait','Departure')][string]$Actor='Chen')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/monk-$($Actor.ToLower())-route-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force "$out/logs"|Out-Null
$canonical="$repo/build-extractors/bin/Release/worldserver.conf";$hash=(Get-FileHash $canonical).Hash
$config=Get-Content $canonical -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'/logs"'))
[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$prior=$env:LEGION_DIALOGUE_CONFIG;$priorCompiler=$env:CL
Push-Location "$repo/build-extractors/bin/Release"
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf";$env:CL="$priorCompiler /DNDEBUG /I`"$repo/src/server/scripts/Legion`""
 & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($repo,$out,"$repo/build-extractors/bin/Release","$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
  & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile "../tests/Monk$($Actor)RouteNative.cpp" -ProbeName "Monk$($Actor)RouteNative" -OutputFile "$out/native.tsv" *> "$out/catalog.log"
  if(!$? -or $LASTEXITCODE){throw "Actual native path probe failed; inspect $out/catalog.log and native.tsv"}
  Get-Content "$out/native.tsv"|Where-Object {$_ -match '^(SEGMENT|PASS)'}
 }
}finally{$env:LEGION_DIALOGUE_CONFIG=$prior;$env:CL=$priorCompiler;Pop-Location}
if((Get-FileHash $canonical).Hash -ne $hash){throw 'Canonical config changed'}
