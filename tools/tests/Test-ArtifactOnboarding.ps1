param([string]$AuditFile="$PSScriptRoot/../../docs/audits/artifact-routes-2026-10-15.json")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/artifact-onboarding"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$source=Get-Content "$repo/src/server/scripts/Legion/ClassHalls/class_hall_artifact_onboarding.cpp" -Raw
$a=$source.IndexOf('namespace');$b=$source.IndexOf('void AddSC_class_hall_artifact_onboarding()',$a)
[IO.File]::WriteAllText("$out/Onboarding.inc",$source.Substring($a,$b-$a))
$audit=Get-Content $AuditFile -Raw | ConvertFrom-Json
if($audit.routes.Count -ne 36 -or $audit.native_race_class_pairs.Count -ne 149){throw 'Full native catalog audit required'}
$pairs=@($audit.native_race_class_pairs | ForEach-Object {"$($_[0])`t$($_[1])"})
[IO.File]::WriteAllLines("$out/races.tsv",[string[]]$pairs)
$maskRows=@(foreach($route in $audit.routes){foreach($q in $route.quest_details){"$($q.id)`t$($route.class_id)`t$($q.class_mask)`t$($q.race_mask)`t$($q.eligible_races -join ',')"}})
[IO.File]::WriteAllLines("$out/masks.tsv",[string[]]$maskRows)
$native=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$functions=@()
foreach($signature in @('bool Player::SatisfyQuestClass(','bool Player::SatisfyQuestRace(')){
 $a=$native.IndexOf($signature);$end=$native.IndexOf('{',$a)+1;$depth=1
 while($depth){if($native[$end] -eq '{'){$depth++};if($native[$end] -eq '}'){$depth--};$end++}
 $functions+=$native.Substring($a,$end-$a).Replace('Player::','')
}
[IO.File]::WriteAllText("$out/QuestMasks.inc",($functions -join "`n"))
foreach($route in $audit.routes){if(!$route.selection_row_present -or !$route.native_chosen_quest_effect){throw 'Choice/native reward spell mismatch'}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/ArtifactOnboardingTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/races.tsv`" `"$out/masks.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Artifact onboarding native handler regression failed'}
