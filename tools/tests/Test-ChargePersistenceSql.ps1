$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$source = Get-Content "$repo/src/server/database/Database/Implementation/CharacterDatabase.cpp" -Raw
$queries = @{}
foreach ($operation in @('SEL','INS','DEL')) {
    $match = [regex]::Match($source, "PrepareStatement\(CHAR_${operation}_CHARACTER_SPELL_CHARGES, `"([^`"]+)`"")
    if (!$match.Success) { throw "Missing production charge query $operation" }
    $queries[$operation] = $match.Groups[1].Value.Replace("'", "''")
}
$fixture = @"
CREATE TEMPORARY TABLE original_charge_schema LIKE character_spell_charges;
CREATE TEMPORARY TABLE character_spell_charges LIKE original_charge_schema;
SET @guid=42,@other=43,@category=10,@spell=1,@consumed=2,@recovery=100750,@regen=1000;
PREPARE save_charges FROM '$($queries.INS)';
PREPARE load_charges FROM '$($queries.SEL)';
PREPARE delete_charges FROM '$($queries.DEL)';
EXECUTE save_charges USING @guid,@category,@spell,@consumed,@recovery,@regen;
EXECUTE save_charges USING @other,@category,@spell,@consumed,@recovery,@regen;
EXECUTE load_charges USING @guid;
EXECUTE delete_charges USING @guid;
SELECT COUNT(*),MIN(guid) FROM character_spell_charges;
"@
$connection = (Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object { $_ -match '^CharacterDatabaseInfo\s*=' }).Split('"')[1].Split(';')
$mysql = Get-ChildItem C:/wamp64/bin/mysql -Recurse -Filter mysql.exe | Select-Object -First 1 -ExpandProperty FullName
$previous = $env:MYSQL_PWD
try {
    $env:MYSQL_PWD = $connection[3]
    $rows = & $mysql --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --skip-column-names --execute=$fixture
    if ($LASTEXITCODE) { throw 'Charge persistence SQL fixture failed' }
    if (($rows -join '|') -ne "10`t1`t2`t100750`t1000|1`t43") { throw 'Charge round trip or character isolation failed' }
} finally { $env:MYSQL_PWD = $previous }
'PASS: production charge INSERT/SELECT/DELETE round trip preserves milliseconds and isolates characters in a temporary table.'
