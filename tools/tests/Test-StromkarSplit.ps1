param([switch]$AddressSanitizer,[string]$RowsFile)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stromkar-split"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 function Section([string]$s,[string]$a,[string]$b){$first=$s.IndexOf($a);$last=$s.IndexOf($b,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native $a"};$s.Substring($first,$last-$first)}
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 [IO.File]::WriteAllText("$out/CreditPlayer.inc",(Section $s 'Player* GetScriptCreditPlayer(' 'Unit* GetScriptCreditCaster('))
 [IO.File]::WriteAllText("$out/CreditCaster.inc",(Section $s 'Unit* GetScriptCreditCaster(' 'SmartScript::SmartScript()').TrimEnd().TrimEnd('}'))
 [IO.File]::WriteAllText("$out/InvokerCast.inc",(Section $s '        case SMART_ACTION_INVOKER_CAST:' '        case SMART_ACTION_ADD_AURA:'))
 [IO.File]::WriteAllText("$out/LinkTail.inc",(Section $s '    if (e.link && e.link != e.event_id)' 'void SmartScript::InstallTemplate('))
 [IO.File]::WriteAllText("$out/CreditAction.inc",(Section $s '        case SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA:' '        case SMART_ACTION_SUMMON_CONVERSATION:'))
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
 [IO.File]::WriteAllText("$out/SmartDeath.inc",(Section $s 'void SmartAI::JustDied(' 'void SmartAI::KilledUnit('))
 $s=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
 [IO.File]::WriteAllText("$out/Count.inc",(Section $s '        case 64:' '        case 3210:'))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 $rowArgument=if($RowsFile){' "'+[IO.Path]::GetFullPath($RowsFile)+'"'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/StromkarSplitTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"$rowArgument`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Stromkar split regression failed'}
}
