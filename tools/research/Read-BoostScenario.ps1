param(
    [string]$Db2Directory = "$PSScriptRoot/../../build-extractors/bin/Release/dbc/enUS",
    [string]$OutputPath
)
$ErrorActionPreference = 'Stop'
$layouts = @{ Scenario='D052232A'; ScenarioStep='201B0EFC'; CriteriaTree='0A1B99C2'; Criteria='A87A5BB9' }

# Read scalar regular WDC1 tables only; fail on unsupported layouts rather
# than interpreting a sparse record or compressed field as an ID.
function Read-WdcTable([string]$Name, [string[]]$Names, [int[]]$StringFields) {
    $b = [IO.File]::ReadAllBytes((Join-Path $Db2Directory "$Name.db2"))
    if ($b.Length -lt 84) { throw "$Name truncated header" }
    if ([BitConverter]::ToUInt32($b,24).ToString('X8') -ne $layouts[$Name]) {
        throw "$Name layout does not match this core's DB2Metadata.h"
    }
    if ([Text.Encoding]::ASCII.GetString($b,0,4) -ne 'WDC1' -or
        ([BitConverter]::ToUInt16($b,44) -band 1)) { throw "Unsupported $Name layout" }
    $count = [BitConverter]::ToUInt32($b,4)
    $fields = [BitConverter]::ToUInt32($b,8)
    $size = [BitConverter]::ToUInt32($b,12)
    $start = 84 + 4 * $fields
    $strings = $start + $count * $size
    $ids = $strings + [BitConverter]::ToUInt32($b,16)
    if ([BitConverter]::ToUInt32($b,64) -ne 4*$count) { throw "$Name must have external IDs" }
    $copies = [BitConverter]::ToUInt32($b,40)
    $meta = $ids + 4*$count + $copies
    $pallet = $meta + [BitConverter]::ToUInt32($b,68)
    if ([BitConverter]::ToUInt32($b,72) -ne 0 -or
        [BitConverter]::ToUInt32($b,68) -ne 24*$fields -or
        $pallet + [BitConverter]::ToUInt32($b,76) -gt $b.Length) {
        throw "$Name unsupported or truncated column storage"
    }
    if ($Names.Count -ne $fields) { throw "$Name field count mismatch" }
    $columns = @()
    for ($f=0; $f -lt $fields; $f++) {
        $p = $meta + 24*$f
        $columns += ,@([int][BitConverter]::ToUInt16($b,$p),
            [int][BitConverter]::ToUInt16($b,$p+2),
            [int][BitConverter]::ToUInt32($b,$p+8), $pallet)
        $pallet += [BitConverter]::ToUInt32($b,$p+4)
    }
    $records = @{}
    for ($i=0; $i -lt $count; $i++) {
        $row = [ordered]@{ID=[BitConverter]::ToUInt32($b,$ids+4*$i)}
        for ($f=0; $f -lt $fields; $f++) {
            $col = $columns[$f]; $bit = $col[0]; $bits = $col[1]; $compression = $col[2]
            if ($compression -notin 0,1,3 -or $bits -gt 32) { throw "$Name unsupported column $f" }
            if ($bit + $bits -gt 8*$size) { throw "$Name column exceeds record" }
            [uint64]$value = 0
            for ($k=0; $k -lt $bits; $k++) {
                $at = $bit+$k
                if ($b[$start+$i*$size+[int][Math]::Floor($at/8)] -band (1 -shl ($at%8))) {
                    $value = $value -bor ([uint64]1 -shl $k)
                }
            }
            if ($compression -eq 3) { $value = [BitConverter]::ToUInt32($b,$col[3]+4*$value) }
            if ($f -in $StringFields) {
                $s = $strings + [int]$value; $end = $s
                while ($end -lt $ids -and $b[$end] -ne 0) { $end++ }
                if ($s -lt $strings -or $end -ge $ids) { throw "$Name invalid string" }
                $row[$Names[$f]] = [Text.Encoding]::UTF8.GetString($b,$s,$end-$s)
            } else { $row[$Names[$f]] = $value }
        }
        $records[$row.ID] = [pscustomobject]$row
    }
    for ($p=$ids+4*$count; $p -lt $meta; $p+=8) {
        $newId=[BitConverter]::ToUInt32($b,$p); $source=[BitConverter]::ToUInt32($b,$p+4)
        if (-not $records.ContainsKey($source)) { throw "$Name missing copy source $source" }
        $copy=$records[$source].PSObject.Copy(); $copy.ID=$newId; $records[$newId]=$copy
    }
    $records.Values
}

$scenarios = @(Read-WdcTable Scenario @('Name','AreaTableID','Flags','Type') @(0) |
    Where-Object { $_.Type -eq 5 -and $_.Name -eq 'Combat Training' })
$steps = @(Read-WdcTable ScenarioStep @('Description','Title','ScenarioID','Supersedes','RewardQuestID','OrderIndex','Flags','CriteriaTreeID','RelatedStep') @(0,1) |
    Where-Object { $_.ScenarioID -in $scenarios.ID })
$trees = @(Read-WdcTable CriteriaTree @('Description','Amount','Flags','Operator','CriteriaID','Parent','OrderIndex') @(0))
$selected = [Collections.Generic.HashSet[uint32]]::new()
foreach ($id in $steps.CriteriaTreeID) { [void]$selected.Add($id) }
do {
    $before = $selected.Count
    foreach ($tree in $trees) {
        if ($selected.Contains($tree.Parent)) { [void]$selected.Add($tree.ID) }
    }
} while ($before -ne $selected.Count)
$trees = @($trees | Where-Object { $selected.Contains($_.ID) })
$criteria = @(Read-WdcTable Criteria @('Asset','StartAsset','FailAsset','ModifierTreeId','StartTimer','EligibilityWorldStateId','Type','StartEvent','FailEvent','Flags','EligibilityWorldStateValue') @() |
    Where-Object { $_.ID -in $trees.CriteriaID })
$missingTrees = @($steps.CriteriaTreeID | Where-Object { $_ -ne 0 -and $_ -notin $trees.ID })
$missingCriteria = @($trees.CriteriaID | Where-Object { $_ -ne 0 -and $_ -notin $criteria.ID })
if ($missingTrees.Count -or $missingCriteria.Count) { throw 'Broken scenario/criteria references' }
$result = [pscustomobject]@{
    Source = 'Local extracted Legion 7.3.5 DB2; not a playable server implementation'
    NumericEncoding = 'Raw unsigned scalar field values; signed fields are not sign-extended'
    Files = @(foreach ($name in @('Scenario','ScenarioStep','CriteriaTree','Criteria')) {
        [pscustomobject]@{ Name="$name.db2"; SHA256=(Get-FileHash (Join-Path $Db2Directory "$name.db2") -Algorithm SHA256).Hash }
    })
    Scenarios = @($scenarios | Sort-Object ID)
    Steps = @($steps | Sort-Object ScenarioID,OrderIndex)
    CriteriaTrees = @($trees | Sort-Object ID)
    Criteria = @($criteria | Sort-Object ID)
}
$json = $result | ConvertTo-Json -Depth 6
if ($OutputPath) {
    [IO.File]::WriteAllText($ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputPath), $json, [Text.UTF8Encoding]::new($false))
    Write-Output "Exported $($scenarios.Count) scenarios, $($steps.Count) steps, $($trees.Count) criterion-tree records and $($criteria.Count) criteria. All selected references resolve."
} else { $json }
