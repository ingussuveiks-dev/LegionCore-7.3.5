# Run in an elevated PowerShell before starting WoW. Successful logins leave
# the bounded memory recording armed until the matching failure or timeout.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ClientLog,
    [ValidateRange(5,1800)][int]$WatchSeconds = 600
)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
$release = Join-Path $repo 'build-extractors/bin/Release'
$serverLog = Join-Path $release 'logs/Server.log'
$root = Join-Path $repo 'build-extractors/runtime-test/handshake-incidents'
$clientLogPath = [IO.Path]::GetFullPath($ClientLog)
$instancePort = (Select-String -LiteralPath "$release/worldserver.conf" -Pattern '^\s*InstanceServerPort\s*=\s*(\d+)\b').Matches.Groups[1].Value
if (!$instancePort) { throw 'Missing InstanceServerPort in the canonical configuration.' }
$session = 'LegionInstanceLogin-' + [guid]::NewGuid().ToString('N')
$recording = @{ Saved = $false }
$profile = "$PSScriptRoot/InstanceLogin.wprp!InstanceLogin"
$wpr = (Get-Command wpr.exe -ErrorAction Stop).Source
& $wpr -start $profile -instancename $session
if ($LASTEXITCODE) { throw "Could not start Windows Performance Recorder (exit $LASTEXITCODE). An elevated PowerShell is required." }
try {
    Write-Host "Watching for a silent login on instance port $instancePort for up to $WatchSeconds seconds."
    $saveTrace = {
        param($incident)
        # Freeze before copying optional client logs and looking up TCP peers.
        & $wpr -status -instancename $session *> "$incident/recording-status.txt"
        if ($LASTEXITCODE) { Write-Warning "WPR status returned $LASTEXITCODE." }
        & $wpr -stop "$incident/instance-login.etl" -skipPdbGen -instancename $session *> "$incident/recording-stop.txt"
        if ($LASTEXITCODE) { throw "Could not save the incident recording (exit $LASTEXITCODE). See $incident/recording-stop.txt" }
        $recording.Saved = $true
        Get-FileHash -LiteralPath "$incident/instance-login.etl" | Select-Object -ExpandProperty Hash |
            Set-Content -LiteralPath "$incident/instance-login.etl.sha256"
    }
    & "$PSScriptRoot/Save-WorldHandshakeIncident.ps1" -Watch -SilentInstancePort $instancePort -OnIncident $saveTrace `
        -ServerLog $serverLog -ClientLog $clientLogPath -OutputDirectory $root -WatchSeconds $WatchSeconds
} finally {
    if (!$recording.Saved) {
        & $wpr -cancel -instancename $session
        if ($LASTEXITCODE) { throw "Could not cancel owned WPR recording $session (exit $LASTEXITCODE)." }
    }
}
