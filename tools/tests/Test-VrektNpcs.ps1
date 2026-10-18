$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/vrekt-npcs"
New-Item -ItemType Directory -Force $out | Out-Null
& "$PSScriptRoot/Test-GreyShoalsControl.ps1" *> "$out/grey-control.log"
Copy-Item "$repo/build-extractors/runtime-test/grey-shoals-control/*.inc" $out
function Export($source,$start,$end,$name){$a=$source.IndexOf($start);$b=$source.IndexOf($end,$a);if($a -lt 0 -or $b -le $a){throw "Missing native boundary $name"};[IO.File]::WriteAllText("$out/$name.inc",$source.Substring($a,$b-$a))}
function Block($source,$start,$name){$a=$source.IndexOf($start);if($a -lt 0){throw "Missing native block $name"};$b=$source.IndexOf('{',$a)+1;$depth=1;while($depth){if($source[$b] -eq '{'){++$depth};if($source[$b] -eq '}'){--$depth};++$b};[IO.File]::WriteAllText("$out/$name.inc",$source.Substring($a,$b-$a))}
$base=Get-Content "$PSScriptRoot/GreyShoalsControlTest.cpp" -Raw
$base=$base.Substring(0,$base.IndexOf('#ifdef CONTROL_BASELINE'))
function Replace-Required($old,$new){if(!$script:base.Contains($old)){throw "Changed base fixture boundary: $old"};$script:base=$script:base.Replace($old,$new)}
Replace-Required 'struct WorldObject{virtual ~WorldObject()=default;};' 'struct WorldObject{virtual ~WorldObject()=default;virtual bool IsNeverVisible(WorldObject const* =nullptr)const{return false;}};'
Replace-Required 'enum DamageEffectType{DIRECT_DAMAGE};' 'enum DamageEffectType{DIRECT_DAMAGE,DOT};'
Replace-Required 'ObjectGuid guid,charmGuid,charmerGuid,minionGuid;' 'CreatureAI* i_AI=nullptr;CreatureAI* i_disabledAI=nullptr;void UpdateCharmAI();bool isPossessed()const{return possessed;}ObjectGuid guid,charmGuid,charmerGuid,minionGuid;'
Replace-Required 'virtual void OnCharmed(bool);' 'virtual void AttackStart(Unit*){}virtual void EnterCombat(Unit*){}virtual void JustDied(Unit*){}virtual void OnCharmed(bool);'
Replace-Required 'CreatureData{int areaId=8297;}' 'CreatureData{int areaId=8362;}'
Replace-Required 'entry=115026,visibility=0;CreatureAI* ai=nullptr;' 'entry=115846,visibility=0;bool m_isUpdate=false,m_AI_locked=false,evading=false;bool IsDelete(){return false;}bool IsPreDelete(){return false;}bool IsInEvadeMode(){return evading;}void Update(uint32);bool IsNeverVisible(WorldObject const*)const override;'
Replace-Required 'CreatureAI* AI(){return ai;}' 'CreatureAI* AI()const{return i_AI;}'
Replace-Required 'Quest* GetQuestTemplate(int){return &quest;}' 'Quest* GetQuestTemplate(int id){return id==44923?&quest:nullptr;}'
[IO.File]::WriteAllText("$out/VrektControlFixture.inc",$base)
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
Export $unit 'void Unit::UpdateCharmAI()' 'bool Unit::isPossessed() const' 'VrektCharmSwap'
$a=$unit.IndexOf('            creature->AI()->DamageTakenAfterScaling(this, damage, damagetype);')
$a=$unit.LastIndexOf('        if (creature->AI())',$a);$b=$unit.IndexOf('        // The original content AI',$a)
$last=$unit.IndexOf('        if (creature->i_disabledAI',$b);$tail=$unit.Substring($last)
Block $tail '        if (creature->i_disabledAI' 'VrektDamageTail'
$damage=$unit.Substring($a,$b-$a)+(Get-Content "$out/VrektDamageTail.inc" -Raw)
[IO.File]::WriteAllText("$out/VrektScaledDamage.inc",$damage)
$creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
Export $creature 'bool Creature::IsNeverVisible(' 'bool Creature::canStartAttack(' 'VrektVisibility'
Export $creature 'void Creature::Update(uint32 diff)' '    volatile uint32 creatureEntry' 'VrektLifecycle'
$alive=$creature.Substring($creature.IndexOf('            // if creature is charmed, switch to charmed AI (and back)'))
Block $alive '            if (NeedChangeAI)' 'VrektRestoreAI'
Block $alive '            if (!IsInEvadeMode() && IsAIEnabled)' 'VrektAliveAI'
$old="$out/old-core";New-Item -ItemType Directory -Force $old | Out-Null
Copy-Item "$out/*.inc" $old
$oldCreature=(& git -C $repo show '1ad46ca1:src/server/game/Entities/Creature/Creature.cpp') -join "`n"
if($LASTEXITCODE){throw 'Pinned old Creature hooks unavailable'}
$oldUnit=(& git -C $repo show '1ad46ca1:src/server/game/Entities/Unit/Unit.cpp') -join "`n"
if($LASTEXITCODE){throw 'Pinned old damage hook unavailable'}
$currentOutput=$out;$out=$old
Export $oldCreature 'bool Creature::IsNeverVisible(' 'bool Creature::canStartAttack(' 'VrektVisibility'
Export $oldCreature 'void Creature::Update(uint32 diff)' '    volatile uint32 creatureEntry' 'VrektLifecycle'
$at=$oldUnit.IndexOf('            creature->AI()->DamageTakenAfterScaling(this, damage, damagetype);')
$start=$oldUnit.LastIndexOf('        if (creature->IsAIEnabled)',$at)
Block ($oldUnit.Substring($start)) '        if (creature->IsAIEnabled)' 'VrektScaledDamage'
$out=$currentOutput
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($variant in @('current','old-core')){
 $include=if($variant -eq 'old-core'){$old}else{$out};$define=if($variant -eq 'old-core'){'/DVREKT_OLD_CORE'}else{''}
 $cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 $define /I`"$include`" /I`"$repo/src/server/scripts/Legion`" `"$PSScriptRoot/VrektNpcTest.cpp`" /Fe`"$out/$variant.exe`" /Fo`"$out/$variant.obj`"`nif errorlevel 1 exit /b 1`n`"$out/$variant.exe`"`nexit /b %errorlevel%`n"
 [IO.File]::WriteAllText("$out/run-$variant.cmd",$cmd);& "$out/run-$variant.cmd";if($LASTEXITCODE){throw "Vrekt native lifecycle/control regression failed ($variant)"}
}
