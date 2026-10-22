param([switch]$Baseline,[string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/quest-turnin-markers")
$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force $output | Out-Null
function Export-Block([string]$Source,[string]$Marker,[string]$File) {
 $start=$Source.IndexOf($Marker);if($start -lt 0){throw "Missing $Marker"}
 $end=$Source.IndexOf('{',$start)+1;$depth=1
 while($depth -gt 0 -and $end -lt $Source.Length){if($Source[$end] -eq '{'){++$depth};if($Source[$end] -eq '}'){--$depth};++$end}
 if($depth){throw "Unbalanced $Marker"}
 [IO.File]::WriteAllText("$output/$File",$Source.Substring($start,$end-$start))
}
$source=if($Baseline){
 $text=& git -C $repo show '0f5222a5:src/server/game/Entities/Player/Player.cpp'
 if($LASTEXITCODE){throw 'Pinned pre-fix Player source unavailable'}
 $text -join "`n"
}else{[IO.File]::ReadAllText("$repo/src/server/game/Entities/Player/Player.cpp")}
Export-Block $source 'QuestGiverStatus Player::GetQuestDialogStatus(' 'Dialog.inc'
$defs=[IO.File]::ReadAllText("$repo/src/server/game/Quests/QuestDef.h")
Export-Block $defs 'enum class QuestGiverStatus :' 'DialogEnums.inc'
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$defines=if($Baseline){'/DEXPECT_BASELINE'}else{''}
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $defines /I`"$output`" `"$PSScriptRoot/QuestTurnInMarkersTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`nif errorlevel 1 exit /b 1`n`"$output/test.exe`"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Quest turn-in production marker regression failed'}
