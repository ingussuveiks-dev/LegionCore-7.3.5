param(
    [string]$RuntimeDirectory = "$PSScriptRoot/../../build-extractors/bin/Release",
    [string]$OutputPath
)
$ErrorActionPreference = 'Stop'
# Read-only evidence extraction. Never export connection strings or account data.
$connection = (Get-Content (Join-Path $RuntimeDirectory 'worldserver.conf') |
    Where-Object { $_ -match '^LoginDatabaseInfo\s*=' }).Split('"')[1].Split(';')
$mysql = Get-ChildItem C:/wamp64/bin/mysql -Recurse -Filter mysql.exe |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $mysql) { throw 'mysql client not found' }
function Read-Rows([string]$Database, [string]$Query) {
    $raw = & $mysql --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$Database --default-character-set=utf8mb4 --xml --execute=$Query
    if ($LASTEXITCODE -ne 0) { throw "Evidence query failed in $Database" }
    [xml]$xml = $raw -join "`n"
    foreach ($row in $xml.resultset.row) {
        $item = [ordered]@{}
        foreach ($field in $row.field) { $item[$field.GetAttribute('name')] = $field.InnerText }
        [pscustomobject]$item
    }
}
$previousPassword = $env:MYSQL_PWD
try {
    $env:MYSQL_PWD = $connection[3]
    $result = [ordered]@{
        Source = 'Read-only local MariaDB legion_world and legion_hotfixes; partial evidence, not a scenario migration'
        Menus = @(Read-Rows legion_world 'SELECT * FROM gossip_menu WHERE MenuID IN(19768,20085,20459) ORDER BY MenuID,TextID')
        Options = @(Read-Rows legion_world 'SELECT * FROM gossip_menu_option WHERE MenuID IN(19768,20085,20459) ORDER BY MenuID,OptionID')
        NpcText = @(Read-Rows legion_world 'SELECT * FROM npc_text WHERE ID IN(29277,29837,30357) ORDER BY ID')
        DialogueSubset = @(Read-Rows legion_hotfixes 'SELECT * FROM broadcast_text WHERE ID IN(54564,108869,111964,117010,118578,118580,120308,120309,121942) OR ID BETWEEN 118210 AND 118240 ORDER BY ID')
        MenuBindings = @(Read-Rows legion_world 'SELECT entry,gossip_menu_id,AIName,ScriptName FROM creature_template WHERE entry IN(105317,100448,114966,112565,107543) ORDER BY entry')
        SmartScripts = @(Read-Rows legion_world 'SELECT * FROM smart_scripts WHERE entryorguid IN(105317,100448,112565,107543) ORDER BY entryorguid,id')
        MenuConditions = @(Read-Rows legion_world 'SELECT * FROM conditions WHERE SourceTypeOrReferenceId IN(14,15) AND SourceGroup IN(19768,20085,20459)')
        ExitSpellDestination = @(Read-Rows legion_world 'SELECT * FROM spell_target_position WHERE id=219912')
    }
    $json = $result | ConvertTo-Json -Depth 7
    if ($OutputPath) {
        [IO.File]::WriteAllText($ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputPath), $json, [Text.UTF8Encoding]::new($false))
        "Exported $($result.Menus.Count) menus, $($result.Options.Count) options and $($result.DialogueSubset.Count) dialogue records."
    } else { $json }
} finally {
    $env:MYSQL_PWD = $previousPassword
}
