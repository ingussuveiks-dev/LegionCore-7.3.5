param(
    [string]$CsvDirectory = "$PSScriptRoot/../../build-extractors/extracted",
    [string]$OutputPath = "$PSScriptRoot/../../sql/updates/world/2026_09_24_317_add_all_client_collectible_items.sql"
)

$ErrorActionPreference = 'Stop'

function Read-ClientTable([string]$Name) {
    $path = Join-Path $CsvDirectory "$Name-26972.csv"
    if (-not (Test-Path -LiteralPath $path)) { throw "Missing $path" }
    Import-Csv -LiteralPath $path
}

function Get-Price([string]$Kind, [int]$ItemId) {
    # The item ID bands follow the original release eras in the 7.3.5 client.
    # Older items receive a lower price; late Legion/Argus items cost most.
    if ($ItemId -ge 151000) { return $(if ($Kind -eq 'Mount') { 45 } else { 25 }) }
    if ($ItemId -ge 125000) { return $(if ($Kind -eq 'Mount') { 35 } else { 20 }) }
    if ($ItemId -ge 105000) { return $(if ($Kind -eq 'Mount') { 28 } else { 15 }) }
    if ($ItemId -ge 78000)  { return $(if ($Kind -eq 'Mount') { 22 } else { 12 }) }
    if ($ItemId -ge 55000)  { return $(if ($Kind -eq 'Mount') { 16 } else { 9 }) }
    if ($ItemId -ge 40000)  { return $(if ($Kind -eq 'Mount') { 12 } else { 7 }) }
    if ($ItemId -ge 23000)  { return $(if ($Kind -eq 'Mount') { 8 } else { 5 }) }
    return $(if ($Kind -eq 'Mount') { 5 } else { 3 })
}

function Quote-Sql([string]$Value) {
    "'" + ($Value -replace "'", "''" -replace '[\r\n]+', ' ') + "'"
}

$mountBySpell = @{}
foreach ($row in (Read-ClientTable Mount)) {
    if ([int]$row.SourceSpellID -gt 0) { $mountBySpell[$row.SourceSpellID] = $row }
}
$petBySpell = @{}
foreach ($row in (Read-ClientTable BattlePetSpecies)) {
    if ([int]$row.SummonSpellID -gt 0) { $petBySpell[$row.SummonSpellID] = $row }
}

$mountDisplay = @{}
foreach ($row in (Read-ClientTable MountXDisplay)) {
    if ([int]$row.CreatureDisplayInfoID -gt 0 -and -not $mountDisplay.ContainsKey($row.MountID)) {
        $mountDisplay[$row.MountID] = [int]$row.CreatureDisplayInfoID
    }
}
$creatureDisplay = @{}
foreach ($row in (Read-ClientTable Creature)) {
    foreach ($column in 'DisplayID_0','DisplayID_1','DisplayID_2','DisplayID_3') {
        if ([int]$row.$column -gt 0) {
            $creatureDisplay[$row.ID] = [int]$row.$column
            break
        }
    }
}
$itemIds = @{}
foreach ($row in (Read-ClientTable Item)) { $itemIds[$row.ID] = $true }

$collectibles = @{}
foreach ($effect in (Read-ClientTable ItemEffect)) {
    if ($effect.TriggerType -ne '6' -or -not $itemIds.ContainsKey($effect.ParentItemID)) { continue }
    $kind = $null
    $model = 0
    if ($mountBySpell.ContainsKey($effect.SpellID)) {
        $kind = 'Mount'
        $mount = $mountBySpell[$effect.SpellID]
        if ($mountDisplay.ContainsKey($mount.ID)) { $model = $mountDisplay[$mount.ID] }
    }
    elseif ($petBySpell.ContainsKey($effect.SpellID)) {
        $kind = 'Pet'
        $pet = $petBySpell[$effect.SpellID]
        if ($creatureDisplay.ContainsKey($pet.CreatureID)) { $model = $creatureDisplay[$pet.CreatureID] }
    }
    if (-not $kind -or -not $model) { continue }
    if ($collectibles.ContainsKey($effect.ParentItemID)) {
        throw "Item $($effect.ParentItemID) has multiple collectible spells"
    }
    $collectibles[$effect.ParentItemID] = [pscustomobject]@{
        Kind = $kind
        ItemId = [int]$effect.ParentItemID
        SpellId = [int]$effect.SpellID
        ModelId = $model
        Name = $null
        Price = Get-Price $kind ([int]$effect.ParentItemID)
        ProductId = 0
        Ordering = 0
    }
}

# ItemSparse has over 100,000 rows. Read only names for eligible items.
Add-Type -AssemblyName Microsoft.VisualBasic
$sparsePath = Join-Path $CsvDirectory 'ItemSparse-26972.csv'
$parser = [Microsoft.VisualBasic.FileIO.TextFieldParser]::new($sparsePath, [Text.Encoding]::UTF8)
try {
    $parser.SetDelimiters(',')
    $parser.HasFieldsEnclosedInQuotes = $true
    $header = $parser.ReadFields()
    $idIndex = [array]::IndexOf($header, 'ID')
    $nameIndex = [array]::IndexOf($header, 'Display_lang')
    if ($idIndex -lt 0 -or $nameIndex -lt 0) { throw 'Unexpected ItemSparse columns' }
    while (-not $parser.EndOfData) {
        $fields = $parser.ReadFields()
        if ($collectibles.ContainsKey($fields[$idIndex])) {
            $collectibles[$fields[$idIndex]].Name = $fields[$nameIndex]
        }
    }
}
finally { $parser.Close() }

$missing = @($collectibles.Values | Where-Object { [string]::IsNullOrWhiteSpace($_.Name) })
foreach ($item in $missing) {
    # Item.db2 alone is not enough to construct a deliverable ItemTemplate.
    $collectibles.Remove([string]$item.ItemId)
}

# Original catalog entries, including two corrected item links and the seven
# additions from update 315. Reprice and reorder these without changing labels.
$existingItemToProduct = @{
    54811=1; 69846=2; 78924=3; 97989=4; 107951=5; 112326=6;
    112327=7; 122469=8; 147901=9;
    49662=10; 49663=11; 70099=12; 78916=13; 98550=14;
    106240=15; 128424=16;
    155656=147; 152816=148; 152789=149; 151623=150;
    151234=151; 147900=152; 141893=153
}
foreach ($itemId in $existingItemToProduct.Keys) {
    if (-not $collectibles.ContainsKey([string]$itemId)) { throw "Existing catalog item $itemId is not a client collectible" }
    $collectibles[[string]$itemId].ProductId = $existingItemToProduct[$itemId]
}

$nextProductId = 154
$ordered = @()
foreach ($kind in 'Mount','Pet') {
    $items = @($collectibles.Values | Where-Object Kind -eq $kind |
        Sort-Object @{Expression='Price';Descending=$true}, @{Expression='ItemId';Descending=$true})
    $ordering = 1
    foreach ($item in $items) {
        if (-not $item.ProductId) { $item.ProductId = $nextProductId++ }
        $item.Ordering = $ordering++
        $ordered += $item
    }
}

if ($ordered.Count -ne $collectibles.Count -or $nextProductId -ge 10000) {
    throw 'Unexpected catalog size or ID range'
}

$sql = [Collections.Generic.List[string]]::new()
$sql.Add('-- Complete item-based mount and battle-pet catalog from the Legion 7.3.5.26972 client.')
$sql.Add('-- Source: client DB2 CSV for Mount, MountXDisplay, BattlePetSpecies,')
$sql.Add('-- Creature, Item, ItemEffect and ItemSparse. Each entry has an existing')
$sql.Add('-- item, a learn-spell effect, a collection record and a preview model.')
$sql.Add('-- The item-ID release bands price newer collectibles above older ones.')
$sql.Add('-- Generated by tools/research/Generate-LegionCollectibleShop.ps1.')
$sql.Add('')
$sql.Add('DROP TEMPORARY TABLE IF EXISTS `_battlepay_collectibles_317`;')
$sql.Add('CREATE TEMPORARY TABLE `_battlepay_collectibles_317` (')
$sql.Add('  `ProductID` INT UNSIGNED NOT NULL PRIMARY KEY,')
$sql.Add('  `GroupID` INT UNSIGNED NOT NULL,')
$sql.Add('  `ItemID` INT UNSIGNED NOT NULL UNIQUE,')
$sql.Add('  `Price` INT UNSIGNED NOT NULL,')
$sql.Add('  `Ordering` INT UNSIGNED NOT NULL,')
$sql.Add('  `ModelID` INT UNSIGNED NOT NULL,')
$sql.Add('  `Name` VARCHAR(255) NOT NULL')
$sql.Add(') ENGINE=MEMORY;')
$sql.Add('')
$sql.Add('START TRANSACTION;')
$sql.Add('')

for ($start = 0; $start -lt $ordered.Count; $start += 200) {
    $end = [Math]::Min($start + 200, $ordered.Count)
    $values = @()
    for ($i = $start; $i -lt $end; $i++) {
        $item = $ordered[$i]
        $groupId = if ($item.Kind -eq 'Mount') { 2 } else { 4 }
        $values += ('({0},{1},{2},{3},{4},{5},{6})' -f $item.ProductId,$groupId,$item.ItemId,$item.Price,$item.Ordering,$item.ModelId,(Quote-Sql $item.Name))
    }
    $sql.Add('INSERT INTO `_battlepay_collectibles_317` (`ProductID`,`GroupID`,`ItemID`,`Price`,`Ordering`,`ModelID`,`Name`) VALUES')
    $sql.Add(($values -join ",`n") + ';')
    $sql.Add('')
}

$sql.Add('UPDATE `battlepay_product` p JOIN `_battlepay_collectibles_317` c ON c.`ProductID` = p.`ProductID`')
$sql.Add('SET p.`NormalPriceFixedPoint` = c.`Price`, p.`CurrentPriceFixedPoint` = c.`Price`')
$sql.Add('WHERE c.`ProductID` < 154;')
$sql.Add('UPDATE `battlepay_shop_entry` s JOIN `_battlepay_collectibles_317` c ON c.`ProductID` = s.`ProductID`')
$sql.Add('SET s.`Ordering` = c.`Ordering` WHERE c.`ProductID` < 154;')
$sql.Add('')
$sql.Add('INSERT INTO `battlepay_display_info` (`DisplayInfoId`,`CreatureDisplayInfoID`,`FileDataID`,`Flags`,`Name1`,`Name2`,`Name3`,`Name4`)')
$sql.Add('SELECT c.`ProductID`,c.`ModelID`,IF(c.`GroupID`=2,4,6),0,c.`Name`,IF(c.`GroupID`=2,''Mount'',''Battle pet''),')
$sql.Add('  ''Use this item to add the collectible to your collection.'','''' FROM `_battlepay_collectibles_317` c WHERE c.`ProductID` >= 154')
$sql.Add('ON DUPLICATE KEY UPDATE `CreatureDisplayInfoID`=VALUES(`CreatureDisplayInfoID`),`FileDataID`=VALUES(`FileDataID`),')
$sql.Add('  `Flags`=VALUES(`Flags`),`Name1`=VALUES(`Name1`),`Name2`=VALUES(`Name2`),`Name3`=VALUES(`Name3`),`Name4`=VALUES(`Name4`);')
$sql.Add('DELETE v FROM `battlepay_display_info_visuals` v JOIN `_battlepay_collectibles_317` c ON c.`ProductID`=v.`DisplayInfoId` WHERE c.`ProductID` >= 154;')
$sql.Add('INSERT INTO `battlepay_display_info_visuals` (`DisplayInfoId`,`DisplayId`,`VisualId`,`ProductName`)')
$sql.Add('SELECT c.`ProductID`,c.`ModelID`,IF(c.`GroupID`=2,4,6),c.`Name` FROM `_battlepay_collectibles_317` c WHERE c.`ProductID` >= 154;')
$sql.Add('')
$sql.Add('INSERT INTO `battlepay_product` (`ProductID`,`NormalPriceFixedPoint`,`CurrentPriceFixedPoint`,`Type`,`ChoiceType`,`Flags`,`DisplayInfoID`,`ScriptName`,`ClassMask`,`WebsiteType`)')
$sql.Add('SELECT c.`ProductID`,c.`Price`,c.`Price`,0,0,0,c.`ProductID`,'''',0,3 FROM `_battlepay_collectibles_317` c WHERE c.`ProductID` >= 154')
$sql.Add('ON DUPLICATE KEY UPDATE `NormalPriceFixedPoint`=VALUES(`NormalPriceFixedPoint`),`CurrentPriceFixedPoint`=VALUES(`CurrentPriceFixedPoint`),')
$sql.Add('  `DisplayInfoID`=VALUES(`DisplayInfoID`);')
$sql.Add('INSERT INTO `battlepay_product_item` (`ID`,`ProductID`,`ItemID`,`Quantity`,`DisplayID`,`PetResult`)')
$sql.Add('SELECT c.`ProductID`,c.`ProductID`,c.`ItemID`,1,0,0 FROM `_battlepay_collectibles_317` c WHERE c.`ProductID` >= 154')
$sql.Add('ON DUPLICATE KEY UPDATE `ItemID`=VALUES(`ItemID`),`Quantity`=VALUES(`Quantity`);')
$sql.Add('INSERT INTO `battlepay_shop_entry` (`EntryID`,`GroupID`,`ProductID`,`Ordering`,`Flags`,`BannerType`,`DisplayInfoID`)')
$sql.Add('SELECT c.`ProductID`,c.`GroupID`,c.`ProductID`,c.`Ordering`,0,0,0 FROM `_battlepay_collectibles_317` c WHERE c.`ProductID` >= 154')
$sql.Add('ON DUPLICATE KEY UPDATE `GroupID`=VALUES(`GroupID`),`Ordering`=VALUES(`Ordering`);')
$sql.Add('')
$sql.Add('COMMIT;')
$sql.Add('DROP TEMPORARY TABLE `_battlepay_collectibles_317`;')

$destination = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputPath)
[IO.File]::WriteAllLines($destination, $sql, [Text.UTF8Encoding]::new($false))
$mountCount = @($ordered | Where-Object Kind -eq 'Mount').Count
$petCount = @($ordered | Where-Object Kind -eq 'Pet').Count
Write-Output "Wrote $destination with $mountCount mounts, $petCount pets, $($nextProductId - 154) new offers; final product ID $($nextProductId - 1); skipped $($missing.Count) items without ItemSparse."
