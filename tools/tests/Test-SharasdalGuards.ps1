param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth -and $last -lt $source.Length){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};if($depth){throw "Unclosed $start"};$source.Substring($first,$last-$first)}
$actorPath='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp';$path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/instance_sharasdal.cpp'
$actor=(Get-Content "$repo/$actorPath" -Raw).Replace("`r",'');$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'')
$oldActor=(& git -C $repo show "c27da5170:$actorPath") -join "`n";if($LASTEXITCODE){throw 'Pinned actor missing'}
$old=(& git -C $repo show "c27da5170:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned instance missing'}
$removed=@'
                            if (Player* targ = me->FindNearestPlayer(100.0f))
                                targ->UpdateAchievementCriteria(CRITERIA_TYPE_SCRIPT_EVENT_2, 49957);
'@
if(!$oldActor.Contains($removed) -or ($actor -replace '\s','') -cne ($oldActor.Replace($removed,'') -replace '\s','')){throw 'Guard repair changed other Sharasdal actors, dialogue, healing, routes or pickup'}
$normalized=$source.Replace((Block $source 'namespace SharasdalGuard'),'')
foreach($header in @('Scenario.h','ScenarioMgr.h')){$normalized=$normalized.Replace("#include `"$header`"",'')}
foreach($method in @('void CreatureDies(','void OnPlayerEnter(','void UpdateGuard(')){$normalized=$normalized.Replace((Block $normalized $method),'')}
$fields=@'
        ObjectGuid guardRecipient;
        std::weak_ptr<uint8> guardAttempt;
        bool guardPending = false, guardBusy = false, guardExhausted = false;
        uint32 guardRetry = 0, guardTries = 0;
'@
$normalized=$normalized.Replace($fields,'').Replace('void Update(uint32 diff) override { UpdateGuard(diff); }','void Update(uint32 diff) override {}')
if(($normalized -replace '\s','') -cne ($old -replace '\s','')){throw 'Other instance/death/initial Brute/graveyard behavior changed'}
if($ScopeOnly){'PASS exact guard death/recipient/lifecycle scope; only waypoint49957 removed; all other actors/initial Brutes/pickup unchanged';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-guards"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
[IO.File]::WriteAllText("$out/Policy.inc",(Block $source 'namespace SharasdalGuard'))
[IO.File]::WriteAllText("$out/Instance.inc",(Block $(if($Baseline){$old}else{$source}) 'struct instance_sharasdal_InstanceMapScript :')+';')
$waypoint=Block (Block $(if($Baseline){$oldActor}else{$actor}) 'void MovementInform(') 'case 8:'
[IO.File]::WriteAllText("$out/Waypoint.inc",'void DispatchWaypoint(Creature* me) { switch (8) {'+$waypoint+' break; } }')
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
[IO.File]::WriteAllText("$out/Owner.inc",(Block $unit 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'))
[IO.File]::WriteAllText("$out/OwnerGuid.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'ObjectGuid GetCharmerOrOwnerGUID() const'))
[IO.File]::WriteAllText("$out/Quest.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'void Player::AddQuestDelayedEvent('))
$kill=Block $unit 'void Unit::Kill('; $dispatch=Block $kill 'if (InstanceScript* script = creature->GetInstanceScript())'
if(!$dispatch.Contains('script->CreatureDies(creature, this);') -or !$dispatch.Contains('script->CreatureDiesForScript(creature, this);')){throw 'Native Kill bridge changed'}
[IO.File]::WriteAllText("$out/Kill.inc",'void Unit::NativeKillDispatch(Creature* creature) {'+$dispatch+'}')
[IO.File]::WriteAllText("$out/ScenarioGate.inc",(Block (Get-Content "$repo/src/server/game/Scenarios/Scenario.cpp" -Raw) 'bool Scenario::CanUpdateCriteria('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/SharasdalGuardsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw "Native guard regression failed: $LASTEXITCODE"}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sharasdal_guards_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical guard implementation missing'}}finally{$env:PYTHONPATH=$previous}
}
