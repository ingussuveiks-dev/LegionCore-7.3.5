param([Parameter(Mandatory=$true)][string]$EvidenceDirectory,[ValidateSet('World','Hotfix')][string]$Database='World',[Parameter(Mandatory=$true)][scriptblock]$Test)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
New-Item -ItemType Directory -Force $EvidenceDirectory|Out-Null
$key="${Database}DatabaseInfo"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match "^$key\s*="}).Split('"')[1].Split(';')
if($c.Count -ne 5 -or $c[4] -notmatch '^\w+$'){throw 'Unexpected world database configuration'}
$mysql='C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe'
$reader='codex_reader_'+[guid]::NewGuid().ToString('N').Substring(0,16)
$secret=[guid]::NewGuid().ToString('N')
$priorPassword=$env:MYSQL_PWD;$priorUser=$env:CODEX_SQL_READER_USER;$priorSecret=$env:CODEX_SQL_READER_PASSWORD
$created=$false
function Admin([string]$sql,[string]$label){
 $env:MYSQL_PWD=$env:CODEX_MYSQL_ADMIN_PASSWORD
 $sql | & $mysql --host=$($c[0]) --port=$($c[1]) --user=root --batch --raw 1> "$EvidenceDirectory/$label.stdout" 2> "$EvidenceDirectory/$label.stderr"
 if($LASTEXITCODE -or (Get-Item "$EvidenceDirectory/$label.stderr").Length){throw "Temporary reader administration failed: $label (private diagnostics retained)"}
}
try{
 Admin "CREATE USER '$reader'@'localhost' IDENTIFIED BY '$secret';" create
 $created=$true
 Admin "GRANT SELECT, CREATE TEMPORARY TABLES ON ``$($c[4])``.* TO '$reader'@'localhost';" grant
 $env:MYSQL_PWD=$secret
 # Controlled denied-write probe: even a failed TEMP setup cannot fall through
 # to production DML. WHERE0 additionally prevents data changes if grants regress.
 $canary=if($Database -eq 'Hotfix'){'hotfix_data'}else{'conditions'}
 "DELETE FROM ``$($c[4])``.$canary WHERE 1=0;"|& $mysql --host=$($c[0]) --port=$($c[1]) --user=$reader --database=$($c[4]) --batch --raw 1> "$EvidenceDirectory/denied.stdout" 2> "$EvidenceDirectory/denied.stderr"
 if(!$LASTEXITCODE -or [IO.File]::ReadAllText("$EvidenceDirectory/denied.stderr") -notmatch 'ERROR 1142 .*DELETE command denied'){throw 'Persistent DELETE privilege was not denied'}
 $env:CODEX_SQL_READER_USER=$reader;$env:CODEX_SQL_READER_PASSWORD=$secret
 & $Test
}finally{
 try{if($created){Admin "DROP USER '$reader'@'localhost';" drop}}finally{$env:MYSQL_PWD=$priorPassword;$env:CODEX_SQL_READER_USER=$priorUser;$env:CODEX_SQL_READER_PASSWORD=$priorSecret}
}
