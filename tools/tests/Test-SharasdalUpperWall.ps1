param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
function Adapt([string]$s,[string]$a,[string]$b){if(!$s.Contains($a)){throw "Upper wall boundary changed: $a"};$s.Replace($a,$b)}
$path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp';$ipath='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/instance_sharasdal.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$instance=(Get-Content "$repo/$ipath" -Raw).Replace("`r",'')
$old=(& git -C $repo show "da3f4b8bb:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned upper wall actor missing'}
$oldInstance=(& git -C $repo show "da3f4b8bb:$ipath") -join "`n";if($LASTEXITCODE){throw 'Pinned upper wall instance missing'}
$scopeSource=$source;$scopeInstance=$instance
if($source.Contains('#include "sharasdal_gauntlet_arrival.h"')){
 & "$PSScriptRoot/Test-SharasdalGauntletArrival.ps1" -ScopeOnly
 if(!$?){throw 'Actual gauntlet delta certificate failed'}
 $scopeSource=(& git -C $repo show "ef0bbcbbf:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-gauntlet actor missing'}
 $scopeInstance=(& git -C $repo show "ef0bbcbbf:$ipath") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-gauntlet instance missing'}
}
$normal=$scopeSource.Replace('        #include "sharasdal_upper_wall.h"','')
$normal=Adapt $normal '            CompleteUpperWall(diff);' ''
$normal=Adapt $normal '            if (!_upperOpened && (_upperMoving || _upperArrived)) CancelUpperWall();' ''
$normal=Adapt $normal 'me->GetMotionMaster()->MovePoint(_upperPoint, -17.96f, 802.05f, 807.66f);' 'me->GetMotionMaster()->MovePoint(1, -17.96f, 802.05f, 807.66f);'
$added=@'
            if (id == 40341 && _upperArrived && guid == _upperRecipient)
            {
                _upperExhausted = false;
                _upperRetry = _upperTries = 0;
            }
'@
$normal=Adapt $normal $added.Replace("`r",'') ''
$added=@'
                        // A late one-second approach must not replace the already
                        // accepted final-wall movement (e.g. a one-hit gauntlet).
                        if (IsUpperGrash() && (_gossipActions & (1u << 4))) return;
'@
$normal=Adapt $normal $added.Replace("`r",'') ''
$added=@'
                if (IsUpperGrash() && id)
                {
                    UpperWallArrival(id);
                    return;
                }
'@
$normal=Adapt $normal $added.Replace("`r",'') ''
$replacement=@'
            if (step == 4 && !SharasdalGrash::Receipt(scenario, 47133))
            {
                player->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 49819);
                if (!SharasdalGrash::Receipt(scenario, 47133))
                    return;
            }
'@
$normal=Adapt $normal "            if (step == 4 && !BindUpperWall(player))`n                return;" $replacement.Replace("`r",'')
if(($normal -replace '\s','') -cne ($old -replace '\s','')){throw 'Other actors/heals/lower walls/gauntlet/deaths/acquisition or registrations changed'}
$normal=$scopeInstance.Replace('        ObjectGuid upperGrash;','').Replace('            if (creature->GetEntry() == 105298 && creature->GetDBTableGUIDLow() == 362092) upperGrash = creature->GetGUID();','')
$normal=$normal.Replace((Block $normal 'void RearmGrashWall('),(Block $oldInstance 'void RearmGrashWall('))
if(($normal -replace '\s','') -cne ($oldInstance -replace '\s','')){throw 'Other instance fields/deaths/credit/entry or initialization changed'}
if($ScopeOnly){'PASS only upper Grash issued point/real final wall/original recipient/re-entry; all lower routes, other actors, acquisition, deaths and earlier instance field order retained';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify repaired Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-upper-wall$(if($Baseline){'-baseline'})"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Test-SharasdalWall.ps1" -PrepareHost -OutputDirectory $out
if(!$?){throw 'Retained native wall host generation failed'}
$actor=Block $(if($Baseline){$old}else{$source}) 'class npc_grash :'
[IO.File]::WriteAllText("$out/Actor.inc",$actor+';')
Copy-Item -LiteralPath "$repo/src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/sharasdal_upper_wall.h" -Destination $out
[IO.File]::WriteAllText("$out/Reentry.inc",(Block $instance 'void RearmGrashWall('))
$hostSource=Get-Content "$out/NativeHost.inc" -Raw
$hostSource=Adapt $hostSource '#include <algorithm>' "#include <algorithm>`n#include <cmath>"
$hostSource=Adapt $hostSource 'enum{MOTION_SLOT_IDLE=0,' 'enum{SPELL_CAST_OK=0,UNIT_STATE_NOT_MOVE=8|32,UNIT_STATE_ROAMING=64,UNIT_STATE_ROAMING_MOVE=128,MOTION_SLOT_IDLE=0,'
$hostSource=Adapt $hostSource 'struct Map{Creature* lower=nullptr;' 'struct Map{Creature* lower=nullptr;Creature* upper=nullptr;'
$hostSource=Adapt $hostSource 'return lower&&lower->guid==guid&&lower->world&&!lower->preDelete?lower:nullptr;' 'for(auto actor:{lower,upper})if(actor&&actor->guid==guid&&actor->world&&!actor->preDelete)return actor;return nullptr;'
$hostSource=Adapt $hostSource 'std::vector<uint32> speech;' 'std::vector<Unit*> attacks;std::vector<uint32> speech;'
$hostSource=Adapt $hostSource 'void AttackStart(Unit*){}' 'void AttackStart(Unit* target){attacks.push_back(target);}'
$hostSource=Adapt $hostSource 'uint32 entry=105298,' 'float x=-181.168f,y=801.922f;bool finalized=true;struct Spline{Creature* owner;bool Finalized()const{return owner->finalized;}} spline{this};Spline* movespline=&spline;uint32 castResult=0,casts=0;std::function<void()> onCast;uint32 CastSpell(Creature* target,uint32 spell,bool triggered){assert(target==this&&spell==209904&&!triggered);++casts;if(onCast)onCast();return castResult;}uint32 entry=105298,'
$hostSource=Adapt $hostSource 'void RemoveFlag(uint32,uint32 flag)' 'void SetFlag(uint32,uint32 flag){flags|=flag;}void RemoveFlag(uint32,uint32 flag)'
$hostSource=Adapt $hostSource 'float GetPositionX()const{return 0;}float GetPositionY()const{return 0;}' 'float GetPositionX()const{return x;}float GetPositionY()const{return y;}'
$hostSource=Adapt $hostSource 'Creature* erunak=nullptr;' 'Creature* lady=nullptr;Creature* erunak=nullptr;'
$hostSource=Adapt $hostSource 'return id==102826?erunak:id==104853?adel:nullptr;' 'return id==102826?erunak:id==104853?adel:id==104856?lady:nullptr;'
$hostSource=Adapt $hostSource 'uint64 dbGuid=134287;' 'bool preDelete=false;bool IsPreDelete()const{return preDelete;}uint32 mapId=1600;uint32 GetMapId()const{return mapId;}uint64 dbGuid=134287;'
[IO.File]::WriteAllText("$out/NativeHost.inc",$hostSource)
$pointSource=Get-Content "$repo/src/server/game/Movement/MovementGenerators/PointMovementGenerator.cpp" -Raw
[IO.File]::WriteAllText("$out/PointFinalize.inc",'template<class T> '+(Block $pointSource 'void PointMovementGenerator<T>::DoFinalize(')+' template<> '+(Block $pointSource 'void PointMovementGenerator<Creature>::MovementInform('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/SharasdalUpperWallTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Actual upper Grash wall regression failed'}
if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$previous=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sharasdal_upper_wall_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Actual upper Grash wall Release implementation missing'}}finally{$env:PYTHONPATH=$previous}}
