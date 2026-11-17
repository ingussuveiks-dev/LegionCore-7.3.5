param([Parameter(Mandatory=$true)][string]$SqlFile,[switch]$VerifyLiveData)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-nonblocking-counts/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
$python='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 & $python "$PSScriptRoot/artifact_nonblocking_counts.py" prepare --output $out --sql-file $SqlFile
 if($LASTEXITCODE){throw 'Count test preparation failed'}
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $cases=@(Get-Content "$out/cases.json" -Raw|ConvertFrom-Json)
 if($VerifyLiveData){$cases+=@([pscustomobject]@{name='live';refusal=$false})}
 foreach($case in $cases){
  $prior=$env:MYSQL_PWD
  try{
   $env:MYSQL_PWD=$db[3]
   $arguments=@("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names')
   if($case.refusal){$arguments+='--force'}
   $stem="$out/$($case.name)"
   $code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments $arguments -InputFile "$stem.sql" -OutputFile "$stem.tsv" -ErrorFile "$stem.stderr"
  }finally{$env:MYSQL_PWD=$prior}
  $errors=[IO.File]::ReadAllText("$stem.stderr")
  if($case.refusal){
   if($code -or $errors -notmatch '\A--------------\r?\nEXECUTE artifact_counts_check\r?\n--------------\r?\n\r?\nERROR 1644 \(45000\) at line \d+: Artifact nonblocking objective context differs; review before updating\r?\n?\z'){throw 'Unexpected refusal diagnostics'}
  }elseif($code -or $errors.Length){throw 'Count SQL check failed'}
 }
 $arguments=@("$PSScriptRoot/artifact_nonblocking_counts.py",'verify','--output',$out,'--sql-file',$SqlFile)
 if($VerifyLiveData){$arguments+='--live'}
 & $python @arguments
 if($LASTEXITCODE){throw 'Count assertions failed'}
}
