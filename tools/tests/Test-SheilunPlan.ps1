param([switch]$Baseline,[switch]$ScopeOnly,[switch]$PrepareHost,[switch]$VerifyNativeCatalog,[switch]$VerifyLiveData,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
$path='src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/Sheylun.cpp';$header='src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/sheilun_taran_intro.h'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$intro=(Get-Content "$repo/$header" -Raw).Replace("`r",'')
$old=(& git -C $repo show "04e5ba3b5:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-plan source missing'}
$oldIntro=(& git -C $repo show "04e5ba3b5:$header") -join "`n";if($LASTEXITCODE){throw 'Pinned pre-plan opening missing'}
if((Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/sheilun_plan.h" -Raw).Contains('#include "sheilun_tactics.h"')){
 & "$PSScriptRoot/Test-SheilunTactics.ps1" -ScopeOnly
 if(!$?){throw 'Current native tactic producer delta failed certification'}
}
. "$PSScriptRoot/SheilunAcquisitionScope.ps1"
$comparison=if($source.Contains('#include "sheilun_acquisition.h"')){Get-SheilunAcquisitionRetainedSource $source}else{$source}
$normal=$comparison.Replace('#include "Map.h"','').Replace('#include "sheilun_plan.h"','')
foreach($role in @('npc_tazhan_chu','npc_sheylun_small_events')){
 $actor=Block $source "class $role :";$prior=Block $old "class $role :"
 $restored=$actor.Replace((Block $actor 'bool OnGossipSelect('),(Block $prior 'bool OnGossipSelect(')).Replace((Block $actor 'void sGossipSelect('),'')
 if($role -eq 'npc_tazhan_chu'){$restored=$restored.Replace((Block $restored 'void DoAction('),(Block $prior 'void DoAction('))}
 if(($restored -replace '\s','') -cne ($prior -replace '\s','')){throw "Unrelated plan actor delta: $role"}
 $normal=$normal.Replace($actor,$restored)
}
if(($normal -replace '\s','') -cne ($old -replace '\s','')){throw 'Sheilun plan changed other actors/story/combat/loot/registrations'}
if(($intro.Replace('    SheilunPlan::Refresh(me);','') -replace '\s','') -cne ($oldIntro -replace '\s','')){throw 'Plan changed opening beyond independent readiness refresh'}
if($ScopeOnly){'PASS exact04e5ba3b5 retained plan delta after explicit ab99b2d2d acquisition/factory certificate: two native menu/index delegates, inert legacy sender/action handlers, receipt-based readiness and lifecycle recovery; independent acquisition wiring is tested separately';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sheilun-plan$(if($Baseline){'-baseline'})"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$selected=if($Baseline){$old}else{$source}
foreach($pair in @(@('Taran','npc_tazhan_chu'),@('Small','npc_sheylun_small_events'))){
 $actor=Block $selected "class $($pair[1]) :"
 [IO.File]::WriteAllText("$out/$($pair[0])Script.inc",(Block $actor 'bool OnGossipSelect('))
 [IO.File]::WriteAllText("$out/$($pair[0])AI.inc",$(if($actor.Contains('void sGossipSelect(')){Block $actor 'void sGossipSelect('}else{''}))
 if($pair[0] -eq 'Taran'){[IO.File]::WriteAllText("$out/Action.inc",(Block $actor 'void DoAction('))}
}
Copy-Item "$repo/src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/sheilun_plan.h" "$out/Plan.inc"
Copy-Item "$repo/src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/sheilun_acquisition_api.h" "$out/sheilun_acquisition_api.h"
if(Test-Path "$repo/src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/sheilun_tactics.h"){Copy-Item "$repo/src/server/scripts/Scenario/Artifacts/Monk/Shey-lun/sheilun_tactics.h" "$out/sheilun_tactics.h"}
$packet=(Get-Content "$repo/src/server/game/Handlers/NPCHandler.cpp" -Raw).Replace("`r",'');$at=$packet.IndexOf('unit->AI()->sGossipSelect(player, packet.GossipID, packet.GossipIndex);');$start=$packet.LastIndexOf('        if (unit)',$at)
if($at -lt 0 -or $start -lt 0){throw 'Native ordinary menu/index dispatch missing'}
[IO.File]::WriteAllText("$out/Dispatch.inc",(Block $packet.Substring($start) 'if (unit)'))
$gossip=Get-Content "$repo/src/server/game/Entities/Creature/GossipDef.cpp" -Raw
$methods=@('uint32 GossipMenu::GetMenuId(','GossipMenuItemContainer const& GossipMenu::GetMenuItems(','void GossipMenu::ClearMenu(','uint32 GossipMenu::GetMenuItemSender(','uint32 GossipMenu::GetMenuItemAction(','void PlayerMenu::ClearMenus(','uint32 PlayerMenu::GetGossipOptionSender(','uint32 PlayerMenu::GetGossipOptionAction(')
[IO.File]::WriteAllText("$out/Gossip.inc",(($methods|ForEach-Object {Block $gossip $_}) -join "`n"))
if($PrepareHost){return}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath;$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/SheilunPlanTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Sheilun plan regression failed'}
if($VerifyNativeCatalog){$prior=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SheilunCatalog.cpp -ProbeName SheilunCatalog -OutputFile "$out/native.tsv";if(!$?){throw 'Native Sheilun catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$prior};$rows=Get-Content "$out/native.tsv";foreach($line in @("CRITERION`t2`t46636`t29443`t92`t48446","CRITERION`t2`t46637`t29444`t92`t48447","CRITERION`t2`t46638`t29445`t92`t48448","CRITERION`t3`t46662`t29455`t92`t48494","EOF`tsheilun")){if($rows -notcontains $line){throw "Native plan contract changed: $line"}}}
if($VerifyLiveData){
 $query=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'actor',entry,gossip_menu_id,AIName,ScriptName FROM creature_template WHERE entry IN(101881,101882,101883,101885,102063) ORDER BY entry;
SELECT 'spawn',guid,id,map,PhaseId FROM creature WHERE id IN(101881,101882,101883,101885,102063) ORDER BY guid;
SELECT 'option',MenuID,OptionID,OptionNpc,OptionBroadcastTextID,BoxCoded,BoxMoney FROM gossip_menu_option WHERE MenuID IN(19235,19225,19226,19227) ORDER BY MenuID,OptionID;
SELECT 'conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId IN(14,15) AND SourceGroup IN(19235,19225,19226,19227);
SELECT 'EOF','sheilun-plan';
'@
 [IO.File]::WriteAllText("$out/live.sql",$query)
 $db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$prior=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$db[3];$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($db[0])","--port=$($db[1])","--user=$($db[2])","--database=$($db[4])",'--batch','--raw','--skip-column-names') -InputFile "$out/live.sql" -OutputFile "$out/live.tsv" -ErrorFile "$out/live.stderr";if($code -or (Get-Item "$out/live.stderr").Length){throw 'Full-EOF plan data readback failed'}}finally{$env:MYSQL_PWD=$prior}
 $expected=@("actor`t101881`t19235`t`tnpc_tazhan_chu","actor`t101882`t19225`t`tnpc_sheylun_small_events","actor`t101883`t19226`t`tnpc_sheylun_small_events","actor`t101885`t19227`t`tnpc_sheylun_small_events","actor`t102063`t0`t`tnpc_sheylun_small_events","spawn`t362831`t101885`t1541`t7000 7002","spawn`t362834`t101883`t1541`t7000 7002","spawn`t362835`t101882`t1541`t7000 7002","spawn`t362836`t102063`t1541`t7000 7002","spawn`t362837`t101881`t1541`t7000","spawn`t362865`t101882`t1541`t7000 7001","spawn`t362867`t101885`t1541`t7000 7001","spawn`t362869`t101883`t1541`t7000 7001","option`t19225`t0`t0`t104452`t0`t0","option`t19225`t1`t0`t104453`t0`t0","option`t19226`t0`t0`t104456`t0`t0","option`t19226`t1`t0`t104457`t0`t0","option`t19227`t0`t0`t104460`t0`t0","option`t19227`t1`t0`t104461`t0`t0","option`t19235`t0`t0`t109100`t0`t0","conditions`t0","EOF`tsheilun-plan")
 if(Compare-Object $expected @(Get-Content "$out/live.tsv")){throw 'Review changed plan bindings/spawns/real menu indices/conditions'}
 'PASS readonly full EOF five bindings/eight exact shared actor spawns/seven native option indices and no extra menu conditions; no persistent writes'
}
if($VerifyReleaseBinary){if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"};$previous=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_sheilun_plan_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Actual Sheilun plan Release implementation missing'}}finally{$env:PYTHONPATH=$previous}}
