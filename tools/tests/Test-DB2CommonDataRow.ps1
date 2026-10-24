param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot verify repaired Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/db2-common-data-row"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/common/DataStores/DB2FileLoader.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "38bad8a2:$path") -join "`n";if($LASTEXITCODE){throw 'Missing pinned common-data baseline'}}
foreach($entry in @(@('VarInt','template<typename T>'),@('RecordId','uint32 DB2FileLoaderRegularImpl::RecordGetId('),@('Packed','uint64 DB2FileLoaderRegularImpl::RecordGetPackedValue('),@('Offset','uint16 DB2FileLoaderRegularImpl::GetFieldOffset('))){$start=$entry[1];if($entry[0] -eq 'VarInt'){$start="template<typename T>`r`nT DB2FileLoaderRegularImpl::RecordGetVarInt(";if(!$source.Contains($start)){$start="template<typename T>`nT DB2FileLoaderRegularImpl::RecordGetVarInt("}};[IO.File]::WriteAllText("$out/$($entry[0]).inc",(Block $source $start))}
foreach($name in @('DB2ColumnCompression','DB2ColumnMeta','DB2PalletValue')){$start=if($name -eq 'DB2ColumnCompression'){"enum class $name"}else{"struct $name"};[IO.File]::WriteAllText("$out/$name.inc",(Block $source $start)+';')}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/DB2CommonDataRowTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native common-data row regression failed'}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_db2_common_data_row_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical common-data row direction missing'}}finally{$env:PYTHONPATH=$previous}
}
