param([switch]$AddressSanitizer,[switch]$Stromkar)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/$(if($Stromkar){'stromkar-execute-jump'}else{'skyhold-departure'})"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){if($s[$b] -eq '{'){++$depth};if($s[$b] -eq '}'){--$depth};++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($out,$repo,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $catalog=if($Stromkar){'StromkarPickupCatalog'}else{'SkyholdTravelCatalog'}
 $prior=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile "$catalog.cpp" -ProbeName $catalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
 $pairs=@(Get-Content "$out/native.tsv" | ForEach-Object {$fields=$_ -split "`t";if($fields[0] -eq 'EFFECT' -and $fields[3] -eq '42'){"$($fields[1]) $($fields[7]) $($fields[2])"}})
 if($Stromkar){
  $native=Get-Content "$out/native.tsv" -Raw
  if(!$native.Contains("EFFECT`t206455`t0`t42`t0`t35`t206456`t46`t0")){throw 'Native Execute jump changed'}
  $pairs=@('206455 206456 0')
 }elseif($pairs.Count -ne 11 -or $pairs -notcontains '203921 203920 0'){throw 'Native Aerylia jump family changed'}
 [IO.File]::WriteAllLines("$out/pairs.tsv",[string[]]$pairs)
 $s=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw;$jump=Block $s 'void Spell::EffectJumpDest('
 [IO.File]::WriteAllText("$out/Speeds.inc",(Block $s 'void Spell::CalculateJumpSpeeds('))
 [IO.File]::WriteAllText("$out/Delay.inc",(Block $s 'DelayCastEvent* Spell::GetSpellTriggerDelay(').Replace('Spell::GetSpellTriggerDelay(','Spell::NativeDelay('))
 $pin=if($Stromkar){'bc4373043'}else{'94438634a'}
 $baseline=(& git -C $repo show "${pin}:src/server/game/Spells/SpellEffects.cpp") -join "`n";if($LASTEXITCODE){throw 'Pinned departure baseline missing'}
 $old=Block $baseline 'void Spell::EffectJumpDest('
 [IO.File]::WriteAllText("$out/NativeCast.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw) 'SpellCastResult Unit::CastSpell(Unit* victim, uint32 spellId, bool triggered,'))
 [IO.File]::WriteAllText("$out/Relocate.inc",(Block (Get-Content "$repo/src/server/game/Entities/Object/Position.h" -Raw) '    void Relocate(float x, float y, float z)'))
 [IO.File]::WriteAllText("$out/MoveJumpPosition.inc",(Block (Get-Content "$repo/src/server/game/Movement/MotionMaster.h" -Raw) '        void MoveJump(Position const& pos,'))
 [IO.File]::WriteAllText("$out/MoveJump.inc",(Block (Get-Content "$repo/src/server/game/Movement/MotionMaster.cpp" -Raw) 'void MotionMaster::MoveJump(float'))
 $s=Get-Content "$repo/src/server/game/Movement/MovementGenerators/PointMovementGenerator.cpp" -Raw
 [IO.File]::WriteAllText("$out/Arrival.inc",(Block $s 'bool EffectMovementGenerator::Update(')+(Block $s 'void EffectMovementGenerator::Finalize(')+(Block $s 'void EffectMovementGenerator::MovementInform('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 foreach($mode in @('baseline','current')){
  $defines=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''};if($mode -eq 'baseline'){$defines+=' /DBASELINE'}
  [IO.File]::WriteAllText("$out/Jump.inc",$(if($mode -eq 'baseline'){$old}else{$jump}))
  [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`ncl /nologo /EHsc /std:c++17 /W4 $defines /I`"$out`" `"$PSScriptRoot/SkyholdDepartureTest.cpp`" /Fe:$mode.exe /Fo:$mode.obj /Fd:$mode.pdb`nif errorlevel 1 exit /b 1`n$mode.exe pairs.tsv`nexit /b %errorlevel%`n")
  & "$out/run.cmd";if($LASTEXITCODE){throw "Skyhold departure $mode failed"}
 }
}
