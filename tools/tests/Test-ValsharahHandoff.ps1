param([string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/valsharah-handoff")
$ErrorActionPreference='Stop'
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=Get-Content "$PSScriptRoot/../../src/server/scripts/Legion/valsharah_tyrande_handoff.cpp" -Raw
$start=$source.IndexOf('namespace ValsharahHandoff');$end=$source.IndexOf('class spell_valsharah_corruption_handoff')
[IO.File]::WriteAllText("$output/ValHandoff.inc",$source.Substring($start,$end-$start))
function Extract-Method([string]$text,[string]$needle){
 $start=$text.IndexOf($needle);if($start -lt 0){throw "Missing production method: $needle"}
 $open=$text.IndexOf('{',$start);$depth=1;$end=$open+1
 while($depth -gt 0 -and $end -lt $text.Length){if($text[$end] -eq '{'){$depth++};if($text[$end] -eq '}'){$depth--};$end++}
 if($depth){throw 'Unbalanced production method'}
 return $text.Substring($start,$end-$start).Replace(' override','')
}
[IO.File]::WriteAllText("$output/ValHandoffScene.inc",(Extract-Method $source 'bool OnTrigger('))
$methods=@('void OnUpdate(','void OnLogin(','void OnMapChanged(','void OnLogout(','void OnQuestReward(')|ForEach-Object {Extract-Method $source $_}
[IO.File]::WriteAllText("$output/ValHandoffPlayer.inc",($methods -join "`n"))
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found'}
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/ValsharahHandoffTest.cpp" /Fe"$output/ValsharahHandoffTest.exe" /Fo"$output/ValsharahHandoffTest.obj"
if errorlevel 1 exit /b 1
"$output/ValsharahHandoffTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Valsharah handoff production regression failed'}
