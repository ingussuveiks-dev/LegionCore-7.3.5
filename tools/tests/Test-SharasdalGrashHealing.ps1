param([switch]$Baseline,[switch]$ScopeOnly,[switch]$VerifyReleaseBinary,[string]$OutputDirectory='')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth -and $last -lt $source.Length){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};if($depth){throw "Unclosed $start"};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'')
$pinned=(& git -C $repo show "e17413eaa:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Grash baseline missing'}
$old=Block $pinned 'class npc_grash :';$current=Block $source 'class npc_grash :'
$oldAI=Block $old 'struct npc_grashAI :';$newAI=Block $current 'struct npc_grashAI :'
foreach($method in @('npc_grashAI(Creature*','void EnterCombat(','void DoAction(','void DamageTakenAfterScaling(','void MovementInform(','void UpdateAI(','CreatureAI* GetAI(')){
 if((Block $current $method) -cne (Block $old $method)){throw "Unrelated Grash method changed: $method"}
}
$normalized=$current
foreach($method in @('uint32 GetData(','void sGossipSelect(','void CancelIntroHeal(','void CompleteIntroHeal(')){$normalized=$normalized.Replace((Block $normalized $method),'')}
$normalized=$normalized.Replace((Block $normalized 'void OnHealApplied('),(Block $old 'void HealReceived('))
foreach($method in @('bool OnGossipHello(','bool OnGossipSelect(','void Reset(','void UpdateLifecycle(')){$normalized=$normalized.Replace((Block $normalized $method),(Block $old $method))}
$fields=@'
        ObjectGuid _introHealer;
        std::weak_ptr<uint8> _introAttempt;
        bool _introReached50 = false, _introReached90 = false, _introBusy = false, _gossipBusy = false;
        uint32 _introRetry = 0, _introTries = 0, _gossipActions = 0;
        FunctionProcessor _healFunctions;
'@
$normalized=$normalized.Replace($fields,'')
if(($normalized -replace '\s','') -cne ($old -replace '\s','')){throw 'Unrelated Grash fields, hooks, movement/combat or factory changed'}
$whole=$source.Replace($current,$old).Replace((Block $source 'namespace SharasdalGrash'),'')
foreach($header in @('ObjectAccessor.h','Scenario.h','ScenarioMgr.h','WaypointManager.h')){$whole=$whole.Replace("#include `"$header`"",'')}
if(($whole -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Other Sharasdal actor/loot/boss/wound/registration changed'}
if($ScopeOnly){'PASS exact Grash final-heal/lease/half-health reply/criteria receipts/raw gossip scope; original combat/damage/routes/actions/other actors retained';return}
if($VerifyReleaseBinary -and $Baseline){throw 'Counterfactual cannot certify Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-grash-healing"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
[IO.File]::WriteAllText("$out/Policy.inc",(Block $source 'namespace SharasdalCompanion')+(Block $source 'namespace SharasdalGrash'))
[IO.File]::WriteAllText("$out/Actor.inc",$(if($Baseline){$old}else{$current})+';')
$core=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
foreach($entry in @(@('Owner','Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'),@('ModifyHealth','int64 Unit::ModifyHealth('))){[IO.File]::WriteAllText("$out/$($entry[0]).inc",(Block $core $entry[1]))}
[IO.File]::WriteAllText("$out/OwnerGuid.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw) 'ObjectGuid GetCharmerOrOwnerGUID() const'))
[IO.File]::WriteAllText("$out/Quest.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw) 'void Player::AddQuestDelayedEvent('))
$heal=Block $core 'int32 Unit::DealHeal(';$last=$heal.IndexOf('    Unit* unit = this;');if($last -lt 0){throw 'Native heal boundary missing'}
[IO.File]::WriteAllText("$out/DealHeal.inc",$heal.Substring(0,$last).Replace(', SpellInfo const* spellProto','')+"    return gain;`n}")
$creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/Notify.inc",(Block $creature 'void Creature::NotifyHealApplied('))
[IO.File]::WriteAllText("$out/Lifecycle.inc",(Block $creature 'if (i_AI || i_disabledAI)'))
$fh=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$fs=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/Functions.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $fh 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$fs.Substring($fs.IndexOf('FunctionProcessor::FunctionProcessor()')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/SharasdalGrashHealingTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw "Grash native heal/gossip regression failed: $LASTEXITCODE"}
if($VerifyReleaseBinary){
 $saved=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$saved";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sharasdal_grash_healing_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Grash final-heal/gossip missing'}}finally{$env:PYTHONPATH=$saved}
}
