param([switch]$Baseline)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot/../..").Path
$path = 'src/server/game/Battlegrounds/Zones/ArenaAshamanesFall.cpp'
$source = if ($Baseline) { (& git -C $root show "484ea0d06:$path") -join "`n" } else {
    Get-Content -LiteralPath "$root/$path" -Raw
}

# Shadow Sight objects captured on map 1552 in TrinityCore commit 39d8f96fdf.
$expected = @(
    @{ Slot = 3; Type = 1; Values = @(3579.0747, 5575.9375, 326.89127, 2.460913, 0.9426413, 0.3338076) }
    @{ Slot = 4; Type = 2; Values = @(3503.2822, 5502.1509, 325.70718, 3.316144, -0.9961939, 0.08716504) }
)
foreach ($orb in $expected) {
    $match = [regex]::Match($source, "AddObject\(BG_VSH_OBJECT_BUFF_$($orb.Slot), BG_VSH_OBJECT_TYPE_BUFF_$($orb.Type), (.*?), BATTLEGROUND_COUNTDOWN_MAX\)")
    if (!$match.Success) { throw "Missing Shadow Sight slot $($orb.Slot)" }
    $values = $match.Groups[1].Value.Split(',') | ForEach-Object { [double]::Parse($_.Trim().TrimEnd('f'), [cultureinfo]::InvariantCulture) }
    $placement = $values[0, 1, 2, 3, 6, 7]
    for ($i = 0; $i -lt $placement.Count; ++$i) {
        if ([math]::Abs($placement[$i] - $orb.Values[$i]) -gt 0.001) {
            throw "Shadow Sight slot $($orb.Slot) differs from its captured placement"
        }
    }
}
'Both Shadow Sight placements match their captured positions and rotations.'
