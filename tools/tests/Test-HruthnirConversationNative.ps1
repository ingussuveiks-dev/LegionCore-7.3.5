param()
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hruthnir-conversation-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
$config=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$originalHash=(Get-FileHash "$repo/build-extractors/bin/Release/worldserver.conf").Hash
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$out.Replace('\','/')+'"'))
[IO.File]::WriteAllText("$out/worldserver.conf",$config)
$previous=$env:LEGION_DIALOGUE_CONFIG
$previousCompiler=$env:CL
try{
 $env:LEGION_DIALOGUE_CONFIG="$out/worldserver.conf"
 $env:CL="$previousCompiler /DNDEBUG /I`"$repo/src/server/scripts/Legion`""
 Push-Location "$repo/build-extractors/bin/Release"
 try{& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/HruthnirConversationNative.cpp -ProbeName HruthnirConversationNative -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw "Native497 execution failed: $out/catalog.log"}}finally{Pop-Location}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$previousCompiler}
if((Get-FileHash "$repo/build-extractors/bin/Release/worldserver.conf").Hash -ne $originalHash){throw 'Canonical configuration changed during native probe'}
if((Test-Path "$out/DBErrors.log") -and (Get-Item "$out/DBErrors.log").Length){throw 'Native497 probe DBErrors require review'}
$evidence=@(Get-Content "$out/native.tsv"|Where-Object {$_ -match '^PASS'})
if($evidence.Count -ne 1 -or !$evidence[0].StartsWith('PASS: actual native497 fails with missing96034')){throw "Native497 success evidence missing: $out/native.tsv"}
$evidence
