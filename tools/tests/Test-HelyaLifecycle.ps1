param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/helya-lifecycle"
New-Item -ItemType Directory -Force -Path $out | Out-Null
function Extract([string]$s,[string]$a,[string]$b){$start=$s.IndexOf($a);if($start -lt 0){throw "Missing $a"};$end=$s.IndexOf($b,$start);if($end -le $start){throw "Missing $b"};return $s.Substring($start,$end-$start)}
$path='src/server/scripts/Legion/TrialOfValor/boss_tov_helya.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'')
[IO.File]::WriteAllText("$out/TrialHeader.inc",(Get-Content "$repo/src/server/scripts/Legion/TrialOfValor/trial_of_valor.h" -Raw))
[IO.File]::WriteAllText("$out/HelyaEnums.inc",(Extract $source 'enum Says' 'Position const Pos_AncientBonethrall'))
foreach($part in @(
 @('Reset','    void Reset(','    void RemoveAllDebaffsOnPlayer('),
 @('Enter','    void EnterCombat(','    void JustDied('),
 @('Start','    void StartEvents(','    void DamageTakenAfterScaling('),
 @('Damage','    void DamageTakenAfterScaling(','    void SpellHit('),
 @('SummonDeath','    void SummonedCreatureDies(','    void SetPower('),
 @('Death','    void JustDied(','    void EnterEvadeMode(')
)){[IO.File]::WriteAllText("$out/Helya$($part[0]).inc",(Extract $source $part[1] $part[2]))}
$baseline=(& git -C $repo show "219ae491:$path") -join "`n"
if($LASTEXITCODE){throw 'Missing pinned pre-repair Helya baseline219ae491'}
[IO.File]::WriteAllText("$out/OldHelyaDamage.inc",(Extract $baseline '    void DamageTaken(' '    void SpellHit('))
$inst=Get-Content "$repo/src/server/scripts/Legion/TrialOfValor/instance_trial_of_valor.cpp" -Raw
[IO.File]::WriteAllText("$out/HelyaVictoryActors.inc",(Extract $inst '        void RestoreHelyaVictoryActors()' '        void OnCreatureCreate('))
$done=$inst.Substring($inst.IndexOf('            case DONE:'))
$done=Extract $done '                case Data::BossIDs::HelyaID:' '                default:'
$done=$done.Substring($done.IndexOf("`n")+1);$done=$done.Substring(0,$done.LastIndexOf('break;'))
[IO.File]::WriteAllText("$out/HelyaDone.inc",$done)
[IO.File]::WriteAllText("$out/HelyaCreate.inc",(Extract $inst '                case Data::Creatures::Helya:' '                default:'))
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$spell=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
$damageIndex=$spell.IndexOf('caster->DealSpellDamage(&damageInfo, true);');$combatIndex=$spell.IndexOf('m_caster->CombatStart(unit, initCombat);')
if($damageIndex -lt 0 -or $combatIndex -lt 0 -or $damageIndex -gt $combatIndex){throw 'Review first spell damage/combat ordering'}
if(!$unit.Contains('creature->AI()->EnterCombat(enemy);')){throw 'Review native combat-entry callback'}
[IO.File]::WriteAllText("$out/NativeDeath.inc",(Extract $unit "        // Call creature just died function" "        // Call instance script."))
[IO.File]::WriteAllText("$out/NativeHealth.inc",(Extract $unit 'uint64 Unit::GetHealth(Unit*' 'uint32 Unit::GetArmor('))
[IO.File]::WriteAllText("$out/NativeScaledHook.inc",(Extract $unit '    damage = uint32(std::min<double>(damage / victim->GetHealthMultiplierForTarget(this), std::numeric_limits<uint32>::max()));' '    // A player charmed by an NPC'))
$creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/NativeMultiplier.inc",(Extract $creature 'float Creature::GetHealthMultiplierForTarget(' 'float Creature::GetBaseDamageForLevel('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/HelyaLifecycleTest.cpp`" /Fe`"$out/Odyn.exe`" /Fo`"$out/Odyn.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/Odyn.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native Helya lifecycle regression failed'}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe";$bytes=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 if(!$bytes.Contains('Trial of Valor Helya: phase two after final scaled damage')){throw 'Canonical Release lacks new Odyn handler marker'}
 [xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
 $release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
 $relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match 'scripts\\Release\\scripts.lib$'} | Select-Object -First 1
 if(!$relative){throw 'Missing actual scripts final-link input'}
 $lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
 if((Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw 'Canonical Release predates scripts library'}
 $map=Get-Content "$repo/build-extractors/bin/Release/worldserver.map" -Raw
 function Symbol([string]$name){
  $index=$map.IndexOf(' '+$name);if($index -lt 0){throw "Missing canonical symbol: $name"}
  $line=$map.Substring($index,$map.IndexOf("`n",$index)-$index)
  $m=[regex]::Match($line,'\s([0-9A-Fa-f]{16})\s+f')
  if(!$m.Success){throw "Missing canonical symbol address: $name"};[Convert]::ToInt64($m.Groups[1].Value,16)
 }
 $binary=[IO.File]::ReadAllBytes($exe);$pe=[BitConverter]::ToInt32($binary,0x3c);$optional=$pe+24
 $imageBase=[BitConverter]::ToInt64($binary,$optional+24);$sections=$optional+[BitConverter]::ToUInt16($binary,$pe+20)
 function Offset([long]$address){
  $relative=$address-$imageBase
  for($i=0;$i -lt [BitConverter]::ToUInt16($binary,$pe+6);$i++){
   $section=$sections+40*$i;$start=[BitConverter]::ToUInt32($binary,$section+12)
   $size=[Math]::Max([BitConverter]::ToUInt32($binary,$section+8),[BitConverter]::ToUInt32($binary,$section+16))
   if($relative -ge $start -and $relative -lt $start+$size){return [long]([BitConverter]::ToUInt32($binary,$section+20)+$relative-$start)}
  };throw 'Address outside canonical PE'
 }
 function Resolve([long]$address){
  for($i=0;$i -lt 10;$i++){$at=Offset $address;if($binary[$at] -ne 0xe9){return $address};$address+=5+[BitConverter]::ToInt32($binary,$at+1)}
  throw 'Unexpected incremental-link thunk cycle'
 }
 function Calls([string]$name){
  $address=Symbol $name;$index=$map.IndexOf(' '+$name);$after=$map.IndexOf("`n",$index)+1
  $tail=$map.Substring($after,[Math]::Min(8192,$map.Length-$after));$end=$address+4096
  foreach($m in [regex]::Matches($tail,'\s([0-9A-Fa-f]{16})\s+f')){$v=[Convert]::ToInt64($m.Groups[1].Value,16);if($v -gt $address){$end=[Math]::Min($end,$v)}}
  $at=Offset $address
  for($i=0;$i -lt $end-$address-4;$i++){
   if($binary[$at+$i] -in @(0xe8,0xe9)){try{Resolve ($address+$i+5+[BitConverter]::ToInt32($binary,$at+$i+1))}catch{}}
  }
 }

 foreach($actor in @('boss_helya_tov')){
  $callback='?DamageTakenAfterScaling@'+$actor+'@@'
  $calls=@(Calls $callback);$target=Resolve (Symbol '?GetHealth@Unit@@QEBA_KPEAV1@@Z')
  if($calls -notcontains $target){throw "Canonical callback lacks target-relative health call: $actor"}
 }
 $woundCalls=@(Calls '?SummonedCreatureDies@boss_helya_tov@@')
 if($woundCalls -notcontains (Resolve (Symbol '?SetHealth@Unit@@QEAAX_KI@Z'))){throw 'Canonical Helya tentacle callback lacks health wound'}
 'PASS: canonical Release marker, actual scripts final-link input, target-health callback and deployed tentacle wound.'
}
