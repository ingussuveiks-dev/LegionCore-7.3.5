param(
    [Parameter(Mandatory)][string]$ClientPath,
    [Parameter(Mandatory)][string[]]$ReportPaths,
    [string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/casc-visuals"
)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../..").Path
$client = (Resolve-Path $ClientPath).Path
if (!(Get-Content "$client/.build.info" | Select-String '7\.3\.5\.26972')) { throw 'Expected client build 7.3.5.26972' }
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$ids = @(@(foreach ($report in $ReportPaths) {
    $data = Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
    $data.referenced_file_ids
    $data.item_visual_file_ids
}) | Where-Object { $_ -gt 0 } | Sort-Object -Unique)
if (!$ids.Count) { throw 'No visual FileDataIDs in the supplied reports' }

# Read sharing alone cannot coexist with the client's existing write handles.
# Modify only private compilation copies; keep all file opens read-only.
foreach ($name in @('CascOpenStorage', 'CascReadFile')) {
    $source = [IO.File]::ReadAllText("$repo/dep/CascLib/src/$name.cpp")
    $old = 'STREAM_FLAG_READ_ONLY | STREAM_PROVIDER_FLAT | BASE_PROVIDER_FILE'
    if (!$source.Contains($old)) { throw "Expected read-only file open missing: $name" }
    $source = $source.Replace($old, 'STREAM_FLAG_READ_ONLY | STREAM_FLAG_WRITE_SHARE | STREAM_PROVIDER_FLAT | BASE_PROVIDER_FILE')
    [IO.File]::WriteAllText("$output/${name}Audit.inc", $source)
}
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$commands = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /MD /D__SYS_ZLIB /I"$repo/dep/zlib" /I"$repo/dep/CascLib/src" /I"$output" "$PSScriptRoot/CascVisualProbe.cpp" /Fe"$output/CascVisualProbe.exe" /Fo"$output/CascVisualProbe.obj" /link "$repo/build-extractors/dep/CascLib/Release/casc.lib" "$repo/build-extractors/dep/zlib/Release/zlib.lib"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/build.cmd", $commands)
& "$output/build.cmd"
if ($LASTEXITCODE -ne 0) { throw 'CASC probe compilation failed; build Release CascLib and zlib first' }

function Get-IndexHashes {
    $files = @(Get-ChildItem -LiteralPath "$client/Data/data" -Filter '*.idx' | Sort-Object Name)
    if (!$files.Count) { throw 'No CASC index files found' }
    foreach ($file in $files) {
        $stream = [IO.File]::Open($file.FullName, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)
        $sha = [Security.Cryptography.SHA256]::Create()
        try { $file.Name + ':' + [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '') }
        finally { $sha.Dispose(); $stream.Dispose() }
    }
}
$before = @(Get-IndexHashes)
$names = @($ids | ForEach-Object { 'File{0:X8}' -f [uint32]$_ })
& "$output/CascVisualProbe.exe" $client @names | Tee-Object -FilePath "$output/files.txt"
$probeExit = $LASTEXITCODE
$after = @(Get-IndexHashes)
if (Compare-Object $before $after) { throw 'Client CASC indexes changed during the audit; retry with a stable client installation' }
if ($probeExit -ne 0) { throw 'One or more client visual files could not be fully read' }
$summary = [ordered]@{ build = '7.3.5.26972'; files_read = $ids.Count; stable_index_files = $before.Count; file_ids = $ids; rendering_verified = $false }
$summary | ConvertTo-Json -Depth 4 | Set-Content "$output/summary.json" -Encoding UTF8
Write-Host "PASS: $($ids.Count) client files fully read; $($before.Count) index hashes unchanged. Rendering still requires the game client."
