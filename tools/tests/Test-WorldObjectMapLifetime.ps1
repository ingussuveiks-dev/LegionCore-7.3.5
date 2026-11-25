param([string]$EvidenceDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/world-object-map")

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$release = "$repo/build-extractors/bin/Release"
$output = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($EvidenceDirectory)

& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $output -Test {
    & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile ../tests/WorldObjectMapLifetimeNative.cpp -ProbeName world-object-map -BuildDirectory $output -BuildOnly
    $previousPath = $env:PATH
    Push-Location $release
    try {
        $env:PATH = "$release;$previousPath"
        & "$output/world-object-map.exe" "$release/dbc/enUS/"
        if ($LASTEXITCODE) { throw "Native map registry regression exited with $LASTEXITCODE" }
    } finally {
        Pop-Location
        $env:PATH = $previousPath
    }
}
