param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
$path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp';$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'')
$old=(& git -C $repo show "c33b7e1f2:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned original reply missing'}
$npc=Block $source 'class npc_erunak_stonespeaker :';$oldNPC=Block $old 'class npc_erunak_stonespeaker :'
$movement=Block $npc 'void MovementInform('; $original=Block $oldNPC 'void MovementInform('
$normal=$movement.Replace('CreatureAI* firstAI = adel->GetDisabledAI() ? static_cast<CreatureAI*>(adel->GetDisabledAI()) : adel->AI();','').Replace('if (!firstAI) return;','').Replace('firstAI->Talk(0);','adel->AI()->Talk(0);').Replace('// This queue belongs to the captured Creature,','').Replace('// but its current AI can change before the reply.','').Replace('if (!adel->IsInWorld() || adel->IsPreDelete() || !adel->IsAlive()) return;','').Replace('CreatureAI* replyAI = adel->GetDisabledAI() ? static_cast<CreatureAI*>(adel->GetDisabledAI()) : adel->AI();','').Replace('if (replyAI) replyAI->Talk(1);','adel->AI()->Talk(1);')
if(($normal -replace '\s','') -cne ($original -replace '\s','')){throw 'Other waypoint/ascent/path/home/speech behavior changed'}
if(($source.Replace($movement,$original) -replace '\s','') -cne ($old -replace '\s','')){throw 'Other Erunak/companions/upper/quest/progress/combat/registration changed'}
foreach($name in @('sharasdal_erunak_entry.h','sharasdal_erunak_upper.h')){$relative='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/'+$name;$prior=(& git -C $repo show "c33b7e1f2:$relative") -join "`n";if($LASTEXITCODE -or (($prior -replace '\s','') -cne ((Get-Content "$repo/$relative" -Raw) -replace '\s',''))){throw 'Entry/upper member policy changed'}}
$relative='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/instance_sharasdal.cpp';$prior=(& git -C $repo show "c33b7e1f2:$relative") -join "`n";if($LASTEXITCODE -or (($prior -replace '\s','') -cne ((Get-Content "$repo/$relative" -Raw) -replace '\s',''))){throw 'Instance admission/death/re-entry changed'}
if($ScopeOnly){'PASS only same-Creature Adelee0/5s1 current-or-original-suspended AI guards; native Unit-owned clock/lifetime, other waypoint/ascent/nearest-player and all other actors/progression retained';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-erunak-reply$(if($Baseline){'-baseline'})"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Test-SharasdalErunakUpper.ps1" -PrepareHost -OutputDirectory $out
if(!$?){throw 'Actual native Erunak/quest/lifecycle/function host preparation failed'}
[IO.File]::WriteAllText("$out/Actor.inc",$(if($Baseline){$oldNPC}else{$npc})+';')
$hostText=Get-Content "$PSScriptRoot/SharasdalErunakUpperTest.cpp" -Raw
$boundary=$hostText.IndexOf('int main(){');if($boundary -lt 0){throw 'Upper native host boundary changed'}
[IO.File]::WriteAllText("$out/ReplyHost.inc",$hostText.Substring(0,$boundary))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/SharasdalErunakReplyTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Actual waypoint Adelee reply regression failed'}
if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$prior=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$prior";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sharasdal_erunak_reply_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Actual Adelee reply guards missing'}}finally{$env:PYTHONPATH=$prior}}
