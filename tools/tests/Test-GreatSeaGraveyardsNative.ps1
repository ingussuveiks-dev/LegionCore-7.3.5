param([string]$EvidenceDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/great-sea-graveyards")

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$release = "$repo/build-extractors/bin/Release"
$output = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($EvidenceDirectory)

& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $output -Test {
    & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/GreatSeaGraveyardsNative.cpp -ProbeName great-sea-graveyards -BuildDirectory $output -BuildOnly
    $previousPath = $env:PATH
    $previousConfig = $env:LEGION_GRAVEYARD_TEST_CONFIG
    Push-Location $release
    try {
        $env:PATH = "$release;$previousPath"
        $env:LEGION_GRAVEYARD_TEST_CONFIG = "$release/worldserver.conf"
        & "$output/great-sea-graveyards.exe" "$release/dbc/enUS/"
        if ($LASTEXITCODE) { throw "Native Great Sea graveyard check exited with $LASTEXITCODE" }
    } finally {
        Pop-Location
        $env:PATH = $previousPath
        $env:LEGION_GRAVEYARD_TEST_CONFIG = $previousConfig
    }
}
