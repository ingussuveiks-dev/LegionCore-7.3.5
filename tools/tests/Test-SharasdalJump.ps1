param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[switch]$VerifyNativeCatalog,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
$spellPath='src/server/game/Spells/SpellEffects.cpp';$motionPath='src/server/game/Movement/MotionMaster.cpp'
$spell=(Get-Content "$repo/$spellPath" -Raw).Replace("`r",'');$motion=(Get-Content "$repo/$motionPath" -Raw).Replace("`r",'')
$oldSpell=(& git -C $repo show "71f30c267:$spellPath") -join "`n";if($LASTEXITCODE){throw 'Pinned native pre-Sharasdal jump missing'}
$oldMotion=(& git -C $repo show "71f30c267:$motionPath") -join "`n";if($LASTEXITCODE){throw 'Pinned native pre-Sharasdal motion missing'}
$spellAdded=@'
    bool sharasdalAcquisition = m_caster->GetMapId() == 1600 &&
        m_spellInfo->Id == 209683 && arrivalCast.SpellId == 209684;
'@
$motionAdded=@'
    bool sharasdalAcquisition = arrivalCast && id == 209683 && arrivalCast->SpellId == 209684 &&
        _owner->IsPlayer() && _owner->GetMapId() == 1600;
'@
$normal=$spell.Replace($spellAdded.Replace("`r",''),'').Replace(' || sharasdalAcquisition','')
if(($normal -replace '\s','') -cne ($oldSpell -replace '\s','')){throw 'Other spell/jump destination/target/speeds/delay/arrival changed'}
$normal=$motion.Replace($motionAdded.Replace("`r",''),'').Replace(' || sharasdalAcquisition','').Replace('(skyholdReturn) ?','skyholdReturn ?')
if(($normal -replace '\s','') -cne ($oldMotion -replace '\s','')){throw 'Other native jump/movement/fallback/slot/constructor changed'}
if($ScopeOnly){'PASS only actual1600 Player209683->209684 self-arrival, positive-launch and captured origin-map gates; all other native spell/motion code retained';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-jump"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
[IO.File]::WriteAllText("$out/Jump.inc",(Block $(if($Baseline){$oldSpell}else{$spell}) 'void Spell::EffectJumpDest('))
[IO.File]::WriteAllText("$out/MoveJump.inc",(Block $(if($Baseline){$oldMotion}else{$motion}) 'void MotionMaster::MoveJump(float'))
[IO.File]::WriteAllText("$out/Speeds.inc",(Block $spell 'void Spell::CalculateJumpSpeeds('))
[IO.File]::WriteAllText("$out/Delay.inc",(Block $spell 'DelayCastEvent* Spell::GetSpellTriggerDelay(').Replace('Spell::GetSpellTriggerDelay(','Spell::NativeDelay('))
[IO.File]::WriteAllText("$out/NativeCast.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw) 'SpellCastResult Unit::CastSpell(Unit* victim, uint32 spellId, bool triggered,'))
[IO.File]::WriteAllText("$out/Relocate.inc",(Block (Get-Content "$repo/src/server/game/Entities/Object/Position.h" -Raw) '    void Relocate(float x, float y, float z)'))
[IO.File]::WriteAllText("$out/MoveJumpPosition.inc",(Block (Get-Content "$repo/src/server/game/Movement/MotionMaster.h" -Raw) '        void MoveJump(Position const& pos,'))
$arrival=Get-Content "$repo/src/server/game/Movement/MovementGenerators/PointMovementGenerator.cpp" -Raw
[IO.File]::WriteAllText("$out/Arrival.inc",(Block $arrival 'bool EffectMovementGenerator::Update(')+(Block $arrival 'void EffectMovementGenerator::Finalize(')+(Block $arrival 'void EffectMovementGenerator::MovementInform('))
$hostSource=Get-Content "$PSScriptRoot/SkyholdDepartureTest.cpp" -Raw;$hostSource=$hostSource.Substring(0,$hostSource.IndexOf('int main('))
$hostSource=$hostSource.Replace('../../src/server/game/Movement/SkyholdDeparture.h',"$($repo.Replace('\','/'))/src/server/game/Movement/SkyholdDeparture.h")
[IO.File]::WriteAllText("$out/NativeHost.inc",$hostSource)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/SharasdalJumpTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Actual Sharasdal destination-jump regression failed'}
if($VerifyNativeCatalog){
 $previous=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SharasdalPickupCatalog.cpp -ProbeName SharasdalJumpCatalog -OutputFile "$out/native.tsv";if(!$?){throw 'Native jump catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
 $rows=Get-Content "$out/native.tsv"
 foreach($row in @("EFFECT`t209683`t0`t0`t42`t125`t209684`t17`t0","EFFECT`t209684`t0`t0`t6`t1255`t0`t1`t0","DURATION`t209684`t15000")){if($rows -notcontains $row){throw "Native jump/scene metadata changed: $row"}}
 'PASS matching26972 native42/209683->209684, database-destination17, scene1255 and15s lifetime; World target-position repair remains separate'
}
if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$previous=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sharasdal_jump_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Actual Sharasdal self-arrival/launch/origin map implementation missing'}}finally{$env:PYTHONPATH=$previous}}
