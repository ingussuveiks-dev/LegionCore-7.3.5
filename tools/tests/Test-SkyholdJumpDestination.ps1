param([switch]$VerifyReleaseBinary,[switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/skyhold-jump-destination"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing block $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
$jump=Block $source 'void Spell::EffectJumpDest('
[IO.File]::WriteAllText("$out/Speeds.inc",(Block $source 'void Spell::CalculateJumpSpeeds('))
[IO.File]::WriteAllText("$out/Delay.inc",(Block $source 'DelayCastEvent* Spell::GetSpellTriggerDelay(').Replace('Spell::GetSpellTriggerDelay(', 'Spell::NativeDelay('))
$baseline=[string]::Join("`n",[string[]](& git -C $repo show '94697469:src/server/game/Spells/SpellEffects.cpp'));if($LASTEXITCODE){throw 'Pinned jump baseline unavailable'}
$baselineJump=Block $baseline 'void Spell::EffectJumpDest('
$source=Get-Content "$repo/src/server/game/Entities/Object/Position.h" -Raw
[IO.File]::WriteAllText("$out/Relocate.inc",(Block $source '    void Relocate(float x, float y, float z)'))
$source=Get-Content "$repo/src/server/game/Movement/MotionMaster.h" -Raw
[IO.File]::WriteAllText("$out/MoveJumpPosition.inc",(Block $source '        void MoveJump(Position const& pos,'))
$source=Get-Content "$repo/src/server/game/Movement/MotionMaster.cpp" -Raw
[IO.File]::WriteAllText("$out/MoveJump.inc",(Block $source 'void MotionMaster::MoveJump(float x,'))
$currentMove=Block $source 'void MotionMaster::MoveJump(float x,'
$oldMove=Block ((git -C $repo show 'aecc4d2a0:src/server/game/Movement/MotionMaster.cpp') -join "`n") 'void MotionMaster::MoveJump(float x,'
$source=Get-Content "$repo/src/server/game/Movement/MovementGenerators/PointMovementGenerator.cpp" -Raw
[IO.File]::WriteAllText("$out/Arrival.inc",(Block $source 'bool EffectMovementGenerator::Update(')+"`n"+(Block $source 'void EffectMovementGenerator::Finalize(')+"`n"+(Block $source 'void EffectMovementGenerator::MovementInform('))
$currentArrival=Get-Content "$out/Arrival.inc" -Raw
$oldArrivalSource=(git -C $repo show 'aecc4d2a0:src/server/game/Movement/MovementGenerators/PointMovementGenerator.cpp') -join "`n"
$oldArrival=(Block $oldArrivalSource 'bool EffectMovementGenerator::Update(')+(Block $oldArrivalSource 'void EffectMovementGenerator::Finalize(')+(Block $oldArrivalSource 'void EffectMovementGenerator::MovementInform(')
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@("SELECT 'delay',spell_trigger,target,caster,effectmask,aura,targetaura FROM spell_trigger_delay WHERE spell_id=192085;SELECT 'target',id,target_map,target_position_x,target_position_y,target_position_z FROM spell_target_position WHERE id=216016;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Skyhold delay read failed'}}finally{$env:MYSQL_PWD=$previous}
if(@($rows|Where-Object {$_ -like "delay`t*"}).Count -ne 1 -or $rows -notcontains "delay`t216016`t0`t0`t7`t0`t0" -or $rows -notcontains "target`t216016`t1479`t1028.64`t7225.23`t100.18"){throw 'Skyhold native arrival graph changed'}
[IO.File]::WriteAllLines("$out/delay.tsv",[string[]]@($rows|Where-Object {$_ -like "delay`t*"}|ForEach-Object {$_.Substring(6)}))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($name in @('baseline','arrival-baseline','test')){
 $baselineMode=$name -eq 'baseline'
 $definition=if($baselineMode){'/DBASELINE'}elseif($name -eq 'arrival-baseline'){'/DARRIVAL_BASELINE'}else{''}
 if($AddressSanitizer){$definition+=' /fsanitize=address /Zi'}
 [IO.File]::WriteAllText("$out/MoveJump.inc",$(if($name -eq 'arrival-baseline'){$oldMove}else{$currentMove}))
 [IO.File]::WriteAllText("$out/Arrival.inc",$(if($name -eq 'arrival-baseline'){$oldArrival}else{$currentArrival}))
 [IO.File]::WriteAllText("$out/Jump.inc",$(if($baselineMode){$baselineJump}else{$jump}))
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/SkyholdJumpDestinationTest.cpp`" /Fe`"$out/$name.exe`" /Fo`"$out/$name.obj`" /Fd`"$out/$name.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/$name.exe`" `"$out/delay.tsv`"`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw "Native Skyhold jump $name failed"}
}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile StromkarPickupCatalog.cpp -ProbeName StromkarPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native Skyhold catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv" -Raw
foreach($expected in @("SKYHOLD_OVERRIDE`t730`t0`t192085","EFFECT`t215997`t0`t6`t293`t730","EFFECT`t192085`t0`t42`t0`t0`t0`t18`t0","EFFECT`t216016`t0`t252","EFFECT`t216016`t1`t16`t0`t39174")){if(!$native.Contains($expected)){throw "Native return data missing: $expected"}}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_skyhold_jump_destination_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Skyhold jump failed'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: Skyhold192085 now sends its intended20-yard lift to actual native MoveJump/MoveTo, retaining delay216016 and ordinary guards/other jumps. Missing finale portal/actor, client ascent/camera and actual teleport/credit remain separate.'
