param([switch]$Baseline,[switch]$AddressSanitizer)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/tuure-boros-greeting"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/$(if($Baseline){'baseline'}else{'current'})" -RuntimeDirectories @($repo,$out) -Test {
 $path='src/server/scripts/Scenario/Artifacts/Priest/Tuure/Tuure.cpp';$source=Get-Content "$repo/$path" -Raw;$actor=Block $source 'struct npc_vindicator_borosAI : ScriptedAI'
 [IO.File]::WriteAllText("$out/EntryPolicy.inc",(Block $source 'namespace TuureIntroHeal'))
 $old=(& git -C $repo show "4fab41ce:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned greeting source missing'}
 $legacy=Block (Block $old 'struct npc_vindicator_borosAI : ScriptedAI') 'void MoveInLineOfSight('
 if(!$legacy.Contains('who->GetTypeId()') -or $legacy.Contains('TuureIntroHeal::Eligible')){throw 'Historical unguarded admission differs'}
 [IO.File]::WriteAllText("$out/Greeting.inc",$(if($Baseline){$legacy}else{Block $actor 'void MoveInLineOfSight('}))
 $fixture=Get-Content "$PSScriptRoot/TuureIntroHealTest.cpp" -Raw
 [IO.File]::WriteAllText("$out/IntroHost.inc",$fixture.Substring(0,$fixture.IndexOf('#include "Policy.inc"')))
 $core=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
 [IO.File]::WriteAllText("$out/Owner.inc",(Block $core 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'))
 [IO.File]::WriteAllText("$out/OwnerGuid.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'ObjectGuid GetCharmerOrOwnerGUID() const'))
 [IO.File]::WriteAllText("$out/Quest.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'void Player::AddQuestDelayedEvent('))
 $heal=Block $core 'int32 Unit::DealHeal(';$last=$heal.IndexOf('    Unit* unit = this;');if($last -lt 0){throw 'Heal fixture boundary missing'}
 [IO.File]::WriteAllText("$out/DealHeal.inc",$heal.Substring(0,$last).Replace(', SpellInfo const* spellProto','')+"    return gain;`n}")
 [IO.File]::WriteAllText("$out/ModifyHealth.inc",(Block $core 'int64 Unit::ModifyHealth('))
 [IO.File]::WriteAllText("$out/Lifecycle.inc",(Block (Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw) 'if (i_AI || i_disabledAI)'))
 $fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
 [IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $def=if($Baseline){'/DBASELINE'}else{''};if($AddressSanitizer){$def+=' /fsanitize=address /Zi'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $def /I`"$out`" `"$PSScriptRoot/TuureBorosGreetingTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Boros greeting regression failed'}
}
