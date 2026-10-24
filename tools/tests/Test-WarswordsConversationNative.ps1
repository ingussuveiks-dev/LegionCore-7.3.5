param()
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-conversation-native/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
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
 try{& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/WarswordsConversationNative.cpp -ProbeName WarswordsConversationNative -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw "Native conversation execution failed: $out/catalog.log"}}finally{Pop-Location}
}finally{$env:LEGION_DIALOGUE_CONFIG=$previous;$env:CL=$previousCompiler}
if((Get-FileHash "$repo/build-extractors/bin/Release/worldserver.conf").Hash -ne $originalHash){throw 'Canonical configuration changed during native probe'}
if((Test-Path "$out/DBErrors.log") -and (Get-Item "$out/DBErrors.log").Length){throw 'Native probe DBErrors require review'}
Get-Content "$out/native.tsv"|Where-Object {$_ -match '^PASS'}
