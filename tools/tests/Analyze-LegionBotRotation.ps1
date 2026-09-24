param(
    [Parameter(Mandatory = $true)]
    [string]$Path
)

$resolved = (Resolve-Path -LiteralPath $Path -ErrorAction Stop).Path
$rows = @(Import-Csv -LiteralPath $resolved -Delimiter "`t")
if ($rows.Count -eq 0) {
    throw "No trace events in $resolved"
}

Write-Output "Trace: $resolved"
Write-Output "Events: $($rows.Count)"

foreach ($group in ($rows | Group-Object actor_guid | Sort-Object Name)) {
    $events = @($group.Group | Sort-Object { [long]$_.uptime_ms })
    $actor = $events[0].actor
    $casts = @($events | Where-Object event -eq 'cast_accepted')
    $failures = @($events | Where-Object event -eq 'cast_failed')
    $hits = @($events | Where-Object event -eq 'dummy_hit')
    $switches = @($events | Where-Object event -eq 'target_acquire')
    $stalls = 0
    $lastAction = $null
    foreach ($event in $events) {
        $now = [long]$event.uptime_ms
        if ($event.event -eq 'target_acquire' -or $event.event -eq 'cast_accepted') {
            $lastAction = $now
        }
        elseif ($event.event -eq 'state' -and $event.result -eq '1' -and
                $event.target_guid -ne '0' -and $null -ne $lastAction -and
                $now - $lastAction -ge 8000) {
            $stalls++
            $lastAction = $now
        }
    }

    Write-Output ""
    Write-Output "$actor [$($group.Name)]: casts=$($casts.Count), dummy_hits=$($hits.Count), target_acquires=$($switches.Count), cast_failures=$($failures.Count), possible_8s_stalls=$stalls"
    foreach ($spell in ($casts | Group-Object spell_id,spell | Sort-Object Count -Descending)) {
        Write-Output "  cast $($spell.Count)x $($spell.Group[0].spell) ($($spell.Group[0].spell_id))"
    }
    foreach ($failure in ($failures | Group-Object spell_id,result | Sort-Object Count -Descending | Select-Object -First 5)) {
        Write-Output "  failed $($failure.Count)x spell=$($failure.Group[0].spell_id) result=$($failure.Group[0].result)"
    }
    foreach ($skip in ($events | Where-Object { $_.event -eq 'cast_skip' -and $_.detail -in @('missing_spell', 'unlearned', 'level') } |
            Group-Object spell_id,detail | Sort-Object Count -Descending | Select-Object -First 5)) {
        Write-Output "  skipped $($skip.Count)x spell=$($skip.Group[0].spell_id) reason=$($skip.Group[0].detail)"
    }
}
