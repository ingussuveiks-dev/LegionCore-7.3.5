param(
    [Parameter(Mandatory=$true)][string]$FixtureDirectory,
    [Parameter(Mandatory=$true)][ValidateSet('checkpoint','finish','reconnect','respawn','expiry')][string]$Stage,
    [switch]$Build
)
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath("$PSScriptRoot/../..")
$fixture = (Resolve-Path -LiteralPath $FixtureDirectory).Path
$database = (Resolve-Path -LiteralPath "$fixture/db").Path
$configPath = (Resolve-Path -LiteralPath "$fixture/worldserver.conf").Path
$options = (Resolve-Path -LiteralPath "$fixture/client.cnf").Path
$release = "$repo/build-extractors/bin/Release"
$catalog = "$repo/build-extractors/runtime-test/legion-legendary-catalog"
$out = "$repo/build-extractors/runtime-test/garrison-continuous/$(Get-Date -Format yyyyMMdd-HHmmss-fff)-$Stage"
if (!$database.StartsWith("$repo\.codex\", [StringComparison]::OrdinalIgnoreCase) -or
    ((Get-Item -LiteralPath $database -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
    throw 'Garrison checks require a disposable data directory under the repository .codex folder'
}
$maria = 'C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mariadb.exe'
$identity = & $maria "--defaults-extra-file=$options" --batch --raw --skip-column-names '--execute=SELECT @@datadir,@@port' --database=fixture_char
if ($LASTEXITCODE -or @($identity).Count -ne 1) { throw 'Could not identify the disposable database' }
$fields = $identity.Split([char]9)
if ($fields.Count -ne 2 -or [IO.Path]::GetFullPath($fields[0]).TrimEnd('\','/') -ne $database -or $fields[1] -eq '3307') {
    throw 'Connection options do not point at the disposable database'
}
$config = Get-Content -LiteralPath $configPath -Raw
$schemas = @{ CharacterDatabaseInfo='fixture_char'; LoginDatabaseInfo='fixture_auth'; WorldDatabaseInfo='legion_world'; HotfixDatabaseInfo='legion_hotfixes' }
foreach ($entry in $schemas.GetEnumerator()) {
    $match = [regex]::Match($config, '(?m)^[ \t]*' + $entry.Key + '[ \t]*=[ \t]*"([^"\r\n]+)"')
    $value = $match.Groups[1].Value.Split(';')
    if ($value.Length -ne 5 -or $value[0] -ne '127.0.0.1' -or $value[1] -ne $fields[1] -or $value[4] -ne $entry.Value) {
        throw 'Native test config contains a database outside the disposable listener'
    }
}
$null = New-Item -ItemType Directory -Path $out,"$out/logs" -Force
$config = [regex]::Replace($config, '(?m)^[ \t]*LogsDir[ \t]*=[^\r\n]*', 'LogsDir = "' + "$out/logs".Replace('\','/') + '"')
$configPath = "$out/worldserver.conf"
[IO.File]::WriteAllText($configPath, $config)
$variables = @{
    LEGION_DIALOGUE_CONFIG = $configPath
    LEGION_GARRISON_FIXTURE_PATH = $database
    LEGION_GARRISON_STAGE = $Stage
    CL = $env:CL + (' /DNDEBUG /Zi /I"{0}/src/server/scripts/Legion" /I"{0}/src/server/scripts/Legion/MawofSouls"' -f $repo)
    _LINK_ = $env:_LINK_ + (' /DEBUG /PDB:"{0}/native.pdb" /MAP:"{0}/native.map"' -f $out)
    PATH = "$release;$env:PATH"
}
$previous = @{}
foreach ($variable in $variables.GetEnumerator()) {
    $previous[$variable.Key] = [Environment]::GetEnvironmentVariable($variable.Key, 'Process')
    [Environment]::SetEnvironmentVariable($variable.Key, $variable.Value, 'Process')
}
try {
    Push-Location $release
    try {
        & "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,$catalog) -Test {
            if ($Build) {
                $probe = @{ SourceFile='../tests/LegionGarrisonContinuousNative.cpp'; ProbeName='LegionGarrisonContinuousNative'; BuildOnly=$true }
                & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" @probe *> "$out/compiler.log"
            }
            & "$catalog/LegionGarrisonContinuousNative.exe" "$release/dbc/enUS/" > "$out/native.tsv" 2> "$out/native-stderr.log"
            if ($LASTEXITCODE) { Get-Content -LiteralPath "$out/native-stderr.log"; throw "Native test failed for $Stage" }
            Get-Content -LiteralPath "$out/native.tsv" | Where-Object { $_ -match '^GARRISON' }
        }
    } finally {
        Pop-Location
    }
} finally {
    foreach ($variable in $previous.GetEnumerator()) {
        [Environment]::SetEnvironmentVariable($variable.Key, $variable.Value, 'Process')
    }
    Remove-Item -LiteralPath $configPath -Force
}
"GARRISON_EVIDENCE $out"
