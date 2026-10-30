param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/halls-odyn-surrender"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Extract([string]$s,[string]$a,[string]$b){$start=$s.IndexOf($a);if($start -lt 0){throw "Missing $a"};$end=$s.IndexOf($b,$start);if($end -le $start){throw "Missing $b"};return $s.Substring($start,$end-$start)}
$path='src/server/scripts/Legion/HallsofValor/boss_odyn.cpp'
$source=Get-Content "$repo/$path" -Raw
[IO.File]::WriteAllText("$out/OdynDamage.inc",(Extract $source '        void DamageTakenAfterScaling(' '        void UpdateAI('))
[IO.File]::WriteAllText("$out/OdynEnterCombat.inc",(Extract $source '        void EnterCombat(' '        void DoAction('))
$baseline=(& git -C $repo show "6813b929:$path") -join "`n"
if($LASTEXITCODE){throw 'Missing pinned pre-repair Odyn baseline6813b929'}
[IO.File]::WriteAllText("$out/OldOdynDamage.inc",(Extract $baseline '        void DamageTaken(' '        void UpdateAI('))
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$spell=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
$damageIndex=$spell.IndexOf('caster->DealSpellDamage(&damageInfo, true);');$combatIndex=$spell.IndexOf('m_caster->CombatStart(unit, initCombat);')
if($damageIndex -lt 0 -or $combatIndex -lt 0 -or $damageIndex -gt $combatIndex){throw 'Review first spell damage/combat ordering'}
if(!$unit.Contains('creature->AI()->EnterCombat(enemy);')){throw 'Review native combat-entry callback'}
[IO.File]::WriteAllText("$out/NativeHealth.inc",(Extract $unit 'uint64 Unit::GetHealth(Unit*' 'uint32 Unit::GetArmor('))
[IO.File]::WriteAllText("$out/NativeScaledHook.inc",(Extract $unit '    damage = uint32(std::min<double>(damage / victim->GetHealthMultiplierForTarget(this), std::numeric_limits<uint32>::max()));' '    // A player charmed by an NPC'))
$creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/NativeMultiplier.inc",(Extract $creature 'float Creature::GetHealthMultiplierForTarget(' 'float Creature::GetBaseDamageForLevel('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/HallsOdynSurrenderTest.cpp`" /Fe`"$out/Odyn.exe`" /Fo`"$out/Odyn.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/Odyn.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native Odyn surrender regression failed'}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe";$bytes=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 if(!$bytes.Contains('Odyn: surrender after final scaled damage')){throw 'Canonical Release lacks new Odyn handler marker'}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 $relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'scripts\\Release\\scripts.lib$'} | Select-Object -First 1
 if(!$relative){throw 'Missing actual scripts final-link input'}
 $lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
 if((Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw 'Canonical Release predates scripts library'}
 'PASS: canonical Release Odyn final-damage marker and actual scripts final-link input.'
}
