param([string]$SqlDirectory='', [switch]$VerifyLiveData)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");if(!$SqlDirectory){$SqlDirectory="$repo/sql/updates/world"}
$out="$repo/build-extractors/runtime-test/ten-thirty-text-data/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";$python='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 & $python -X utf8 "$PSScriptRoot/prepare_ten_thirty_text_tests.py" prepare --sql-directory $SqlDirectory --output $out;if($LASTEXITCODE){throw 'Fixture generation failed'}
 $cases=[IO.File]::ReadAllText("$out/cases.json")|ConvertFrom-Json;$db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 if($VerifyLiveData){$cases+=@([pscustomobject]@{path="$out/live.sql";refusal=$false})}
 foreach($case in $cases){$p=[IO.Path]::Combine([IO.Path]::GetDirectoryName($case.path),[IO.Path]::GetFileNameWithoutExtension($case.path));$prior=$env:MYSQL_PWD
  try{$env:MYSQL_PWD=$db[3];$arguments=@("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names');if($case.refusal){$arguments+='--force'};$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $arguments -InputFile $case.path -OutputFile "$p.tsv" -ErrorFile "$p.stderr"}finally{$env:MYSQL_PWD=$prior}
  @{exit=$code;expected_refusal=[bool]$case.refusal}|ConvertTo-Json|Set-Content "$p-status.json";$errors=[IO.File]::ReadAllText("$p.stderr")
  if($case.refusal){if($code -or $errors -notmatch '\A--------------\r?\nEXECUTE text_guard\r?\n--------------\r?\n\r?\nERROR 1644 \(45000\) at line \d+: Text batch\d+ differs; review before updating\r?\n?\z'){throw 'Exact refusal differs; inspect capture'}}elseif($code -or $errors.Length){throw 'Unexpected SQL error; inspect capture'}
 }
 $arguments=@('-X','utf8',"$PSScriptRoot/prepare_ten_thirty_text_tests.py",'verify','--sql-directory',$SqlDirectory,'--output',$out);if($VerifyLiveData){$arguments+='--live'};& $python @arguments;if($LASTEXITCODE){throw 'Actual full-schema/live assertions failed'}
}
