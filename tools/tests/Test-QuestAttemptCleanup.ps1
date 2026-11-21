param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/quest-attempt-cleanup")
$ErrorActionPreference='Stop'
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
& "$PSScriptRoot/Export-QuestDelayedEvent.ps1" -OutputDirectory $output
$source=Get-Content "$PSScriptRoot/../../src/server/game/Entities/Player/Player.cpp" -Raw
$methods=@(@('void Player::RemoveActiveQuest(','void Player::RemoveRewardedQuest('),@('bool Player::TakeQuestSourceItem(','bool Player::GetQuestRewardStatus('))
$helper=$source.IndexOf('class QuestItemRemoval')
$helperEnd=$source.IndexOf('void Player::RewardQuest(',$helper)
$text=@($source.Substring($source.LastIndexOf('namespace',$helper),$helperEnd-$source.LastIndexOf('namespace',$helper)))
$text+=foreach($method in $methods){$start=$source.IndexOf($method[0]);$end=$source.IndexOf($method[1],$start);$source.Substring($start,$end-$start)}
[IO.File]::WriteAllText("$output/QuestCleanup.inc",($text -join "`n"))
$handler=Get-Content "$PSScriptRoot/../../src/server/game/Handlers/QuestHandler.cpp" -Raw
$abandon=[regex]::Match($handler,'(?s)void WorldSession::HandleQuestLogRemoveQuest\(.*?(?=void WorldSession::HandleQuestConfirmAccept\()').Value
if ([regex]::Matches($abandon,'TakeQuestSourceItem\(').Count -ne 1 -or
    $abandon.IndexOf('TakeQuestSourceItem(') -gt $abandon.IndexOf('RemoveActiveQuest(')) {
    throw 'The abandonment packet must remove quest source items exactly once before changing the log'
}
$reward=[regex]::Match($source,'(?s)void Player::RewardQuest\(.*?(?=\nvoid Player::)').Value
if ([regex]::Matches($reward,'RemoveQuestRewardItems\(').Count -ne 1 -or
    $reward.Contains('TakeQuestSourceItem(') -or $reward.Contains('DestroyItemCount(')) {
    throw 'Quest reward must apply one combined set of item removals'
}
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual C++ compiler not found'}
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/QuestAttemptCleanupTest.cpp" /Fe"$output/QuestAttemptCleanupTest.exe" /Fo"$output/QuestAttemptCleanupTest.obj"
if errorlevel 1 exit /b 1
"$output/QuestAttemptCleanupTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Quest cleanup production regression failed'}
