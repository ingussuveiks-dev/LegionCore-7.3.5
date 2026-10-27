param([switch]$AddressSanitizer)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/kingslayers-return"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){if($s[$b] -eq '{'){++$depth};if($s[$b] -eq '}'){--$depth};++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($out,$repo) -Test {
 Copy-Item "$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/kingslayers_return.h" "$out/Return.inc"
 [IO.File]::WriteAllText("$out/CriteriaTypes.inc",(Block (Get-Content "$repo/src/server/game/DataStores/DBCEnums.h" -Raw) 'enum CriteriaTypes')+';')
 $s=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp" -Raw
 $space=Block $s 'namespace KingslayersAcquisition'
 [IO.File]::WriteAllText("$out/Acquisition.inc",'namespace KingslayersAcquisition {'+(Block $space 'uint32 Quest(')+(Block $space 'bool Leaf(')+'}')
 $player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
 $header=Get-Content "$repo/src/server/game/Entities/Player/Player.h" -Raw
 [IO.File]::WriteAllText("$out/QuestDelay.inc",(Block $player 'void Player::AddQuestDelayedEvent('))
 [IO.File]::WriteAllText("$out/Pending.inc",(Block $header 'bool HasPendingTeleport() const'))
 [IO.File]::WriteAllText("$out/DelayScope.inc",(Block $header 'class ScopedTeleportDelay')+';')
 $a=$player.IndexOf('            SetDelayedTeleportFlag(IsCanDelayTeleport());',$player.IndexOf('// far teleport to another map'));$b=$player.IndexOf('            EndMovieMovement();',$a)
 if($a -lt 0 -or $b -le $a){throw 'Native far teleport deferral missing'}
 [IO.File]::WriteAllText("$out/TeleportDelay.inc",$player.Substring($a,$b-$a))
 $s=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
 $use=Block $s 'void GameObject::Use('
 $a=$use.IndexOf('    if (Player* playerUser');$b=$use.IndexOf('    switch (GetGoType())',$a)
 $prefix=$use.Substring($a,$b-$a)
 $goober=Block $use 'case GAMEOBJECT_TYPE_GOOBER:'
 $a=$use.LastIndexOf('    if (!spellId)')
 if($a -lt 0){throw 'Native GO spell dispatch missing'}
 [IO.File]::WriteAllText("$out/Use.inc",'void GameObject::Use(Unit* user){ Unit* spellCaster=user;uint32 spellId=0;bool triggered=false;'+$prefix+'switch(GetGoType()){'+$goober+'}'+$use.Substring($a))
 $cast=Block $s 'SpellCastResult GameObject::CastSpell('
 $a=$cast.IndexOf('    if (goInfo->type == GAMEOBJECT_TYPE_TRAP && target && target->IsPlayer())')
 [IO.File]::WriteAllText("$out/GoCast.inc",$cast.Substring(0,$a)+'return SPELL_FAILED_ERROR;}')
 $s=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
 [IO.File]::WriteAllText("$out/SendEvent.inc",(Block $s 'void Spell::EffectSendEvent('))
 $tele=Block $s 'void Spell::EffectTeleportUnits('
 $a=$tele.IndexOf('    bool checkDest = true;');$b=$tele.IndexOf('    // post effects for TARGET_DEST_DB')
 if($a -lt 0 -or $b -le $a){throw 'Native generic teleport route missing'}
 [IO.File]::WriteAllText("$out/Teleport.inc",'void Spell::EffectTeleportUnits(SpellEffIndex effIndex){if(effectHandleMode != SPELL_EFFECT_HANDLE_HIT_TARGET || !unitTarget || unitTarget->isInFlight())return;'+$tele.Substring($a,$b-$a)+'}')
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/KingslayersReturnTest.cpp`" /Fe:test.exe /Fo:test.obj /Fd:test.pdb`nif errorlevel 1 exit /b 1`ntest.exe`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Kingslayers return native regression failed'}
}
