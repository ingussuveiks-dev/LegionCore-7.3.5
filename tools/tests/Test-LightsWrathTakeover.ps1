param([switch]$AddressSanitizer,[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/lights-wrath-takeover"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){if($s[$b] -eq '{'){++$depth};if($s[$b] -eq '}'){--$depth};++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo,$out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 Copy-Item "$repo/src/server/scripts/Legion/ClassHalls/lights_wrath_takeover.h" "$out/Takeover.inc"
 $s=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuraDefines.h" -Raw
 $modes=(Block $s 'enum AuraEffectHandleModes')+';'
 $s=Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw
 [IO.File]::WriteAllText("$out/NativeModes.inc",$modes+(Block $s 'enum AuraRemoveMode')+';')
 $s=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
 $note=$s.IndexOf('// Content policies must still run');$first=$s.LastIndexOf('m_AI_locked = true;',$note);$last=$s.IndexOf('m_AI_locked = false;',$note)
 if($first -lt 0 -or $last -lt $first){throw 'Native locked lifecycle handoff missing'}
 [IO.File]::WriteAllText("$out/NativeLifecycle.inc",$s.Substring($first,$last+20-$first))
 $s=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
 [IO.File]::WriteAllText("$out/QuestDelay.inc",(Block $s 'void Player::AddQuestDelayedEvent('))
 $s=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuras.cpp" -Raw
 [IO.File]::WriteAllText("$out/NativeDuration.inc",(Block (Block $s 'void Aura::Update(uint32 diff, Unit* caster)') 'if (m_duration > 0)'))
 $s=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
 $a=$s.IndexOf('// remove expired auras - do that after updates');$b=$s.IndexOf('// m_auraUpdateIterator can be updated',$a)
 [IO.File]::WriteAllText("$out/NativeExpiry.inc",$s.Substring($a,$b-$a))
 $s=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
 [IO.File]::WriteAllText("$out/NativeSendEvent.inc",(Block $s 'void Spell::EffectSendEvent('))
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 $a=$s.IndexOf('        case SMART_ACTION_UPDATE_ACHIEVEMENT_CRITERIA:');$b=$s.IndexOf('        case SMART_ACTION_SUMMON_CONVERSATION:',$a)
 if($a -lt 0 -or $b -lt $a){throw 'Native legacy205 consumer missing'}
 [IO.File]::WriteAllText("$out/NativeLegacyCredit.inc",$s.Substring($a,$b-$a))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/LightsWrathTakeoverTest.cpp`" /Fe:test.exe /Fo:test.obj`nif errorlevel 1 exit /b 1`ntest.exe`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native takeover regression failed'}
 if($VerifyNativeCatalog){
  $prior=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile LightsWrathCatalog.cpp -ProbeName LightsWrathCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
  $native=@(Get-Content "$out/native.tsv")
  foreach($row in @("EFFECT`t207949`t0`t6`t26`t0`t0`t1`t25","EFFECT`t207949`t1`t6`t23`t0`t207960`t1`t0","DURATION`t207949`t30000`t30000","DURATION`t207956`t30000`t30000","EFFECT`t207996`t1`t61`t0`t49811`t0`t1`t0","CRITERION`t1065`t4`t47117`t29656`t92`t49810","CRITERION`t1065`t5`t47119`t29657`t92`t49811","LFG`t1228`t1065`t1583`t12","LFG`t1241`t1101`t1583`t12")){if($native -notcontains $row){throw 'Native control/expiry/previous victory/shared instance contract differs'}}
  if($native|Where-Object {$_ -like "SHARED_USE`t*"}){throw 'Additional native summon/trigger source requires shared-use review'}
 }
}
