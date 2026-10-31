param([switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
$output=[IO.Path]::GetFullPath("$PSScriptRoot/../../build-extractors/runtime-test/currency-loot-response")
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=Get-Content "$PSScriptRoot/../../src/server/game/Loot/LootMgr.cpp" -Raw
$start=$source.IndexOf('void Loot::BuildLootResponse(')
$start=$source.IndexOf('    QuestItemMap const& lootPlayerCurrencies =',$start)
$end=$source.IndexOf('    QuestItemMap const& lootPlayerQuestItems =',$start)
if($start -lt 0 -or $end -lt 0){throw 'Currency response section not found'}
[IO.File]::WriteAllText("$output/CurrencyResponse.inc",$source.Substring($start,$end-$start))
$helperStart=$source.IndexOf('static bool IsLootItemVisibleTo(')
$helperEnd=$source.IndexOf('// return true if there is any FFA, quest or conditional item for the player.',$helperStart)
if($helperStart -lt 0 -or $helperEnd -le $helperStart){throw 'Actual native loot visibility helper boundary missing'}
[IO.File]::WriteAllText("$output/Visibility.inc",$source.Substring($helperStart,$helperEnd-$helperStart))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'MSVC not found'}
$sanitizer=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 $sanitizer /I"$output" "$PSScriptRoot/CurrencyLootResponseTest.cpp" /Fe"$output/CurrencyLootResponseTest.exe" /Fo"$output/CurrencyLootResponseTest.obj" /Fd"$output/CurrencyLootResponseTest.pdb" /link /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"$output/CurrencyLootResponseTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Currency loot response regression failed'}
