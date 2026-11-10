param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/native-sql-leading-options/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";[void][IO.Directory]::CreateDirectory($out)
$helper="$PSScriptRoot/Invoke-NativeSqlFiles.ps1"
if($Baseline){$old=(& git -C $repo show '3d9746b2c:tools/tests/Invoke-NativeSqlFiles.ps1') -join "`n";if($LASTEXITCODE){throw 'Pinned SQL helper missing'};$helper="$out/pinned-helper.ps1";[IO.File]::WriteAllText($helper,$old)}
[IO.File]::WriteAllText("$out/client.ini","[client]`ndefault-character-set=utf8mb4`n")
[IO.File]::WriteAllText("$out/input.sql",'')
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $prior=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$null
  foreach($client in @('C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe','C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe')){
   $prefix=if($client -like '*mariadb*'){'mariadb'}else{'mysql'};$index=0
   foreach($selector in @('--no-defaults',"--defaults-file=$out/client.ini","--defaults-extra-file=$out/client.ini")){
    $arguments=@($selector,'--help')
    $code=& $helper -Executable $client -Arguments $arguments -InputFile "$out/input.sql" -OutputFile "$out/$prefix-$index.stdout" -ErrorFile "$out/$prefix-$index.stderr"
    $stdout=[IO.File]::ReadAllText("$out/$prefix-$index.stdout");$stderr=[IO.File]::ReadAllText("$out/$prefix-$index.stderr")
    if($Baseline -and $prefix -eq 'mariadb'){if($code -isnot [int] -or $stderr -notmatch 'unknown (option|variable)'){throw 'Expected original native selector rejection missing'}}
    elseif($code -or $stderr.Length -or $stdout -notmatch 'Usage:'){throw 'Native leading selector failed'}
    ++$index
   }
  }
 }finally{$env:MYSQL_PWD=$prior}
 if($Baseline){'PASS expected-broken three MariaDB defaults selectors rejected when streaming option is prepended;three MySQL8 compatibility controls retained'}else{'PASS six actual MariaDB/MySQL native defaults selectors precede streaming option;actual parser/help exit0 and empty stderr,no connection or credentials. SQL streaming covered separately.'}
}
