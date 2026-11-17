param([Parameter(Mandatory)][string]$DataDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/fire-lyandra-phase"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 function Block($text,$name){$a=$text.IndexOf($name);if($a -lt 0){throw "Missing native $name"};$b=$text.IndexOf('{',$a);$end=$b+1;$depth=1;while($depth){if($text[$end] -eq '{'){$depth++};if($text[$end] -eq '}'){$depth--};$end++};$text.Substring($a,$end-$a)}
 function Section($text,$first,$next){$a=$text.IndexOf($first);$b=$text.IndexOf($next,$a);if($a -lt 0 -or $b -le $a){throw "Missing native $first"};$text.Substring($a,$b-$a)}
 $smart=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw;$ai=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartAI.cpp" -Raw
 $creatureAI=Get-Content "$repo/src/server/game/AI/CreatureAI.cpp" -Raw;$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
 $creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw;$auras=Get-Content "$repo/src/server/game/Spells/Auras/SpellAuraEffects.cpp" -Raw
 $homeSource=Get-Content "$repo/src/server/game/Movement/MovementGenerators/HomeMovementGenerator.cpp" -Raw
 $pieces=[ordered]@{
  CancelPending=(Block $smart 'void SmartScript::CancelPendingActions()');LinkedReceipt=(Block $smart 'bool SmartScript::HasCompletedLinkedCast(')
  ScriptUpdate=(Block $smart 'void SmartScript::OnUpdate(')
  HomeFinalize=((Block $homeSource 'void HomeMovementGenerator<Creature>::DoFinalize(Creature& owner)') -replace '^void HomeMovementGenerator<Creature>::DoFinalize\(Creature& owner\)\s*\{','' -replace '\}\s*$','')
  UpdateVictim=(Block $creatureAI 'bool CreatureAI::UpdateVictim()');EnterEvade=(Block $creatureAI 'bool CreatureAI::_EnterEvadeMode()')
  SmartUpdate=(Block $ai 'void SmartAI::UpdateAI(');SmartAttack=(Block $ai 'void SmartAI::AttackStart(');SmartEvade=(Block $ai 'void SmartAI::EnterEvadeMode()')
  SelectVictimBody=((Block $unit 'Unit* Creature::SelectVictim()') -replace '^Unit\* Creature::SelectVictim\(\)\s*\{','' -replace '\}\s*$','')
  AttackPrefix=(Section ((Block $unit 'bool Unit::Attack(') -replace '^bool Unit::Attack\([^\r\n]*\)\s*\{','') '    if (!victim || victim == this)' '    if (m_attacking)')
  CombatStop=(Block $unit 'void Unit::CombatStop(');SetCombat=(Block $unit 'void Unit::SetInCombatWith(')
  Unattackable=(Block $auras 'void AuraEffect::HandleModUnattackable(');Regenerate=(Block $creature 'void Creature::RegenerateHealth()')
  RegenTick=(Section $creature '            bool bInCombat = isInCombat()' '            m_regenTimer = isAnySummons()')
 }
 # The regen call site is compiled on its actor; preserve its expression exactly.
 $pieces.RegenTick='auto isInCombat=[&](){return actor.isInCombat();};auto getVictim=[&](){return actor.getVictim();};auto IsInEvadeMode=[&](){return actor.IsInEvadeMode();};auto IsPolymorphed=[&](){return actor.IsPolymorphed();};auto RegenerateHealth=[&](){actor.RegenerateHealth();};'+$pieces.RegenTick
 $all='';foreach($name in $pieces.Keys){[IO.File]::WriteAllText("$out/$name.inc",$pieces[$name]);$all+=$pieces[$name]}
 $headers=@('src/common/Common.h','src/server/game/AI/SmartScripts/SmartScriptMgr.h','src/server/game/AI/SmartScripts/SmartAI.h','src/server/game/Entities/Unit/Unit.h','src/server/game/Entities/Unit/UnitDefines.h','src/server/game/Entities/Player/Player.h','src/server/game/Entities/Creature/Creature.h','src/server/game/Entities/Pet/PetDefines.h','src/server/game/Entities/Object/ObjectDefines.h','src/server/game/Spells/Auras/SpellAuraDefines.h','src/server/game/Spells/SpellInfo.h','src/server/game/Miscellaneous/SharedDefines.h','src/server/game/Movement/MotionMaster.h','src/server/game/World/World.h')|ForEach-Object {Get-Content "$repo/$_" -Raw}
 $header=[string]::Join("`n",$headers);$constantBuilder=[Text.StringBuilder]::new();[void]$constantBuilder.Append('constexpr uint32 UNIT_FIELD_FLAGS=0,UNIT_FIELD_FLAGS_3=3;'+(Block $header 'enum MovementSlot')+';')
 $emitted=@{'UNIT_FIELD_FLAGS'=$true;'UNIT_FIELD_FLAGS_3'=$true;'MOTION_SLOT_ACTIVE'=$true;'M_PI'=$true}
 function Emit($name){
  if($emitted.ContainsKey($name)){return};$emitted[$name]=$true
  $m=[regex]::Match($header,"(?m)^\s*$name\s*=\s*([^,`r`n]+)")
  if(!$m.Success){$m=[regex]::Match($header,"(?m)^#define\s+$name\s+([^`r`n]+)")}
  if(!$m.Success){throw "Missing native enum/macro $name"}
  $value=($m.Groups[1].Value -replace '//.*','').Trim()
  foreach($dep in @([regex]::Matches($value,'\b[A-Z][A-Z0-9_]+\b')|ForEach-Object Value)){Emit $dep}
  [void]$constantBuilder.Append("constexpr auto $name=$value;`n")
 }
 $names=@([regex]::Matches($all+(Get-Content "$repo/src/server/scripts/Legion/ClassHalls/fire_lyandra_phase.h" -Raw)+(Get-Content "$PSScriptRoot/FireLyandraPhaseTest.cpp" -Raw),'\b(?:SMART_|UNIT_|SPELL_|AURA_|REACT_|MOTION_|WAYPOINT_|POINT_|ATTACK_|PET_|RATE_|CREATURE_)[A-Z0-9_]+\b|\bIN_MILLISECONDS\b|\bINTERACTION_DISTANCE\b')|ForEach-Object Value|Sort-Object -Unique)
 foreach($name in $names){Emit $name}
 [IO.File]::WriteAllText("$out/Constants.inc",$constantBuilder.ToString())
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $data=[IO.Path]::GetFullPath($DataDirectory)
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" /I`"$repo/src/server/scripts/Legion/ClassHalls`" `"$PSScriptRoot/FireLyandraPhaseTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$data/terminal-smart.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Lyandra phase regression failed'}
}
