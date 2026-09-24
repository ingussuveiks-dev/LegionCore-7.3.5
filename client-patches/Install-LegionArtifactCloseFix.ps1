[CmdletBinding()]
param(
    [string] $ClientPath = 'D:\wow\Legion7.3.5'
)

$ErrorActionPreference = 'Stop'
$source = Join-Path $PSScriptRoot 'LegionArtifactCloseFix'
$target = Join-Path $ClientPath 'Interface\AddOns\LegionArtifactCloseFix'

if (-not (Test-Path -LiteralPath (Join-Path $ClientPath 'Wow-64_Patched.exe') -PathType Leaf)) {
    throw "Legion 7.3.5 client not found: $ClientPath"
}

New-Item -ItemType Directory -Path $target -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $source 'LegionArtifactCloseFix.toc') -Destination $target -Force
Copy-Item -LiteralPath (Join-Path $source 'LegionArtifactCloseFix.lua') -Destination $target -Force
Write-Host "Installed LegionArtifactCloseFix in $target. Reload the client UI once to load it."
