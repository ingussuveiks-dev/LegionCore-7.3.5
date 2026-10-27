param([switch]$AddressSanitizer,[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/bilaal-death"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing native block $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){if($s[$b] -eq '{'){++$depth};if($s[$b] -eq '}'){--$depth};++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo,$out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 Copy-Item "$repo/src/server/scripts/Scenario/Artifacts/Rogue/Dreadblades/bilaal_death.h" "$out/Bilaal.inc"
 $s=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
 [IO.File]::WriteAllText("$out/QuestObjective.inc",(Block $s 'int32 Player::GetQuestObjectiveData(uint32'))
 [IO.File]::WriteAllText("$out/QuestStorage.inc",(Block $s 'int32 Player::GetQuestObjectiveData(Quest const*'))
 $s=Get-Content "$repo/src/server/game/Entities/Creature/Creature.h" -Raw
 [IO.File]::WriteAllText("$out/RewardAllowed.inc",(Block $s 'bool IsDamageEnoughForLootingAndReward() const')+"`n"+(Block $s 'void LowerPlayerDamageReq('))
 $s=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
 $a=$s.IndexOf('player->RewardPlayerAndGroupAtKill(victim, false);',$s.IndexOf('void Unit::Kill('))
 $b=$s.IndexOf('if (creature->IsAIEnabled)',$a);$c=$s.IndexOf('script->CreatureDies(creature, this);',$b)
 if($a -lt 0 -or $b -le $a -or $c -le $b){throw 'Native reward/AI-independent instance death order differs'}
 $dispatch=Block $s.Substring($b) 'if (creature->IsAIEnabled)'
 $instance=Block $s.Substring($b) 'if (InstanceScript* script = creature->GetInstanceScript())'
 [IO.File]::WriteAllText("$out/NativeDeathDispatch.inc",$dispatch+"`n"+$instance)
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/BilaalDeathTest.cpp`" /Fe:test.exe /Fo:test.obj`nif errorlevel 1 exit /b 1`ntest.exe`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Bilaal native death regression failed'}
 if($VerifyNativeCatalog){
  $prior=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile LightsWrathCatalog.cpp -ProbeName LightsWrathCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
  $catalog=@(Get-Content "$out/native.tsv")
  foreach($row in @("CRITERION`t1065`t4`t47117`t29656`t92`t49810","CRITERION`t1101`t4`t47995`t29656`t92`t49810")){if($catalog -notcontains $row){throw "Native Bilaal victory contract differs: $row"}}
  foreach($row in @($catalog|Where-Object {$_ -like "BILAAL_SOURCE`t*"})){if(($row -split "`t")[2] -ne '90'){throw 'Unreviewed native shared Bilaal effect/producer'}}
 }
}
