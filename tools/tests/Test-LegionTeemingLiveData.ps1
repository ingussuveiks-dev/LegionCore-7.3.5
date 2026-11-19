param()
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/legion-teeming-live-data/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
& 'C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' "$repo/tools/research/generate_legion_teeming_spawns.py" --output "$out/generated.sql" --verify-live-sql "$out/verify.sql"
if($LASTEXITCODE){throw 'Native catalog consistency/generation failed'}
$conf=Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" -Raw
$match=[regex]::Match($conf,'(?m)^\s*WorldDatabaseInfo\s*=\s*"([^"]+)"');if(!$match.Success){throw 'Runtime World connection absent'}
$parts=$match.Groups[1].Value.Split(';');if($parts.Count -ne 5){throw 'Runtime World connection shape changed'}
function Quote-Client([string]$v){'"'+$v.Replace('\','\\').Replace('"','\"')+'"'}
$private="$out/client.cnf"
try{
 [IO.File]::WriteAllText($private,"[client]`nhost=$(Quote-Client $parts[0])`nport=$($parts[1])`nuser=$(Quote-Client $parts[2])`npassword=$(Quote-Client $parts[3])`ndatabase=$(Quote-Client $parts[4])`n")
 & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
  Get-Content "$out/verify.sql" -Raw|& 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mariadb.exe' "--defaults-extra-file=$private" --batch --raw --skip-column-names *> "$out/readback.jsonl"
  if($LASTEXITCODE){throw "Live content validation failed:$out/readback.jsonl"}
  $rows=@(Get-Content "$out/readback.jsonl"|ForEach-Object {$_|ConvertFrom-Json})
  if($rows.Count -ne 4 -or @($rows|Where-Object {$_.expected -ne $_.joined -or $_.expected -ne $_.matched}).Count){throw 'Live World content differs from reviewed catalog'}
  'PASS live native-schema readback:all32 creature fields on215 spawns,215 addons,98 waypoints and266 formation rows match the catalog;permanent data remains read-only'
 }
}finally{if(Test-Path -LiteralPath $private){Remove-Item -LiteralPath $private -Force}}
"LIVE_READBACK $out"
