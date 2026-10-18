$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/grey-shoals-rotation"
New-Item -ItemType Directory -Force $out | Out-Null
function Export([string]$file,[string]$start,[string]$end,[string]$name){
 $source=Get-Content "$repo/$file" -Raw;$a=$source.IndexOf($start);$b=$source.IndexOf($end,$a)
 if($a -lt 0 -or $b -le $a){throw "Missing production boundary $name"}
 [IO.File]::WriteAllText("$out/$name.inc",$source.Substring($a,$b-$a))
}
Export 'src/server/game/Entities/Creature/Creature.cpp' 'bool Creature::IsNeverVisible(' 'bool Creature::canStartAttack(' 'RotationCreatureVisibility'
Export 'src/server/game/Entities/GameObject/GameObject.cpp' 'bool GameObject::IsNeverVisible(' 'uint8 GameObject::GetLevelForTarget(' 'RotationGoVisibility'
Export 'src/server/game/Entities/Creature/Creature.cpp' 'void Creature::Update(uint32 diff)' '    volatile uint32 creatureEntry' 'RotationLifecycle'
Export 'src/server/scripts/Legion/zone_azsuna.cpp' 'class go_grey_shoals_supplies' 'void AddSC_azsuna()' 'RotationGoFactory'
Export 'src/server/game/AI/CreatureAI.h' '    // Runs before alive/corpse/respawn' '    // Called for reaction at stopping attack' 'RotationCreatureDefaults'
Export 'src/server/game/AI/CoreAI/GameObjectAI.h' '        virtual bool CanBeVisibleFor(' '        virtual void InitializeAI()' 'RotationGoDefaults'
Export 'src/server/game/AI/SmartScripts/SmartScript.cpp' 'bool SmartScript::IsSmart(Creature* c)' 'bool SmartScript::IsSmartGO(' 'RotationSmartAdmission'
$native=Get-Content "$repo/src/server/game/Entities/Object/Object.cpp" -Raw
$a=$native.IndexOf('    if (obj->IsNeverVisible(this) || CanNeverSee(obj) || obj->CanNeverSee2(this))')
$b=$native.IndexOf('    bool corpseVisibility',$a)
if($a -lt 0 -or $b -le $a){throw 'Missing native visibility filter admission'}
[IO.File]::WriteAllText("$out/RotationVisibilityDispatch.inc",$native.Substring($a,$b-$a))
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$a=$source.IndexOf('            creature->AI()->DamageTakenAfterScaling(this, damage, damagetype);')
$b=$source.IndexOf("`n        }",$a)
if($a -lt 0 -or $b -le $a){throw 'Missing native scaled-damage dispatch'}
[IO.File]::WriteAllText("$out/RotationScaledDamage.inc",$source.Substring($a,$b-$a))
# Pin the semantic placement, not just the presence of the new hook.
$source=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
$start=$source.IndexOf('void Creature::Update(uint32 diff)')
$hook=$source.IndexOf('AI()->UpdateLifecycle(diff);',$start)
foreach($boundary in @('uint32 updateDiff','if (IsAIEnabled && TriggerJustRespawned)','switch (m_deathState)','if (!IsInEvadeMode() && IsAIEnabled)')){
 if($hook -lt $start -or $hook -ge $source.IndexOf($boundary,$start)){throw "Lifecycle hook is too late for $boundary"}
}
# A correctly scoped AI must also be constructible on the first DB grid load.
$source=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
$start=$source.IndexOf('bool GameObject::LoadGameObjectFromDB(')
$identity=$source.IndexOf('m_DBTableGuid = guid;',$start);$create=$source.IndexOf('if (!Create(',$start)
if($start -lt 0 -or $identity -lt $start -or $create -lt $start -or $identity -ge $create){throw 'GO spawn identity assigned after AI factory construction'}
$source=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
$start=$source.IndexOf('void Creature::AddToWorld()')
$end=$source.IndexOf('void Creature::RemoveFromWorld()',$start)
$initialize=$source.IndexOf('AIM_Initialize();',$start)
if($start -lt 0 -or $initialize -lt $start -or $end -le $initialize){throw 'Creature grid addition no longer initializes the scoped AI'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /I`"$out`" /I`"$repo/src/server/scripts/Legion`" `"$PSScriptRoot/GreyShoalsRotationTest.cpp`" /Fe`"$out/Rotation.exe`" /Fo`"$out/Rotation.obj`"`nif not %errorlevel%==0 exit /b %errorlevel%`n`"$out/Rotation.exe`"`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd)
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Grey Shoals production rotation regression failed'}
'PASS: production actor/rack factories and lifecycle, actual native visibility functions, Creature::Update prefix and scaled-damage dispatcher. Map/visibility packets, SmartAI combat services and respawn internals remain fixtures.'
