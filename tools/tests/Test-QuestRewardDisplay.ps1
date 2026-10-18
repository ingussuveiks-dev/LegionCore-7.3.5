$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/quest-reward-display"
New-Item -ItemType Directory -Force $out | Out-Null
function Extract([string]$source,[string]$first,[string]$last,[string]$file){$a=$source.IndexOf($first);$b=$source.IndexOf($last,$a);if($a -lt 0 -or $b -le $a){throw "Reward display boundary missing: $file"};[IO.File]::WriteAllText("$out/$file.inc",$source.Substring($a,$b-$a))}
$s=Get-Content "$repo/src/server/game/Quests/QuestRewardDisplay.h" -Raw
Extract $s 'inline bool IsQuestRewardDisplayOnly(' '#endif' 'RewardDisplayHelper'
$s=Get-Content "$repo/src/server/game/Spells/SpellMgr.cpp" -Raw
Extract $s 'bool SpellMgr::IsSpellValid(' 'bool SpellMgr::IsSpellForbidden(' 'RewardDisplayValidity'
$s=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
Extract $s '        for (uint32 i = 0; i < QUEST_REWARD_DISPLAY_SPELL_COUNT;' '        if (qinfo->RewardMailTemplateId)' 'RewardDisplayLoader'
$s=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
Extract $s '    if (quest->RewardSpell > 0)' '    if (quest->QuestSortID > 0)' 'RewardDisplayAward'
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/QuestRewardDisplayTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native display-only quest reward regression failed'}
