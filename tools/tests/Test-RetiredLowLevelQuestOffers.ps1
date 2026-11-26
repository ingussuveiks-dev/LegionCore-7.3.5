param(
    [string]$ConfigFile = "$PSScriptRoot/../../build-extractors/bin/Release/worldserver.conf",
    [string]$MariaDb = 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mariadb.exe',
    [switch]$Baseline
)

$ErrorActionPreference = 'Stop'
$config = Get-Content -LiteralPath $ConfigFile -Raw
$connection = [regex]::Match($config,'(?m)^WorldDatabaseInfo\s*=\s*"([^"]+)"')
if (!$connection.Success) { throw 'World database setting is missing' }
$hostName,$port,$user,$password,$database = $connection.Groups[1].Value.Split(';')
$query = @'
START TRANSACTION READ ONLY;
SELECT COUNT(*) FROM creature_queststarter WHERE (id,quest) IN((1416,325),(15278,8329));
SELECT COUNT(*) FROM creature_questender WHERE (id,quest) IN((311,325),(43010,8329));
SELECT COUNT(*) FROM quest_template WHERE ID IN(325,8329);
SELECT COUNT(*) FROM creature_queststarter WHERE (id,quest) IN((1416,353),(15278,8325),(15278,8326),(15278,8327),(15278,31170));
ROLLBACK;
'@
$prior = $env:MYSQL_PWD
try {
    $env:MYSQL_PWD = $password
    $result = $query | & $MariaDb --no-defaults --protocol=tcp "--host=$hostName" "--port=$port" "--user=$user" "--database=$database" --batch --raw --skip-column-names
    if ($LASTEXITCODE -ne 0) { throw 'Read-only quest offer verification could not complete' }
} finally {
    $env:MYSQL_PWD = $prior
}

$expected = if ($Baseline) { '2,2,2,5' } else { '0,2,2,5' }
if (($result -join ',') -ne $expected) {
    throw "Unexpected counts for retired offers, receivers, templates and neighboring offers ($($result -join ','))"
}
Write-Output "PASS retired offers, historical receivers, quest templates and neighboring offers ($expected)"
