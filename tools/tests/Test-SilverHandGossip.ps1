param([switch]$Baseline,[switch]$Second,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned baseline cannot verify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/$(if($Second){'silver-hand-second-gossip'}else{'silver-hand-gossip'})"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$reference=if($Second){'cf8ab051'}else{'357347d7'};$source=(& git -C $repo show "${reference}:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned gossip baseline unavailable'}}
$identity=if($Second){'npc_travar_second'}else{'npc_travar_first'}
[IO.File]::WriteAllText("$out/EntrancePolicy.inc",$(if($Second -and $Baseline){''}else{Block $source $(if($Second){'namespace SilverHandProcession'}else{'namespace SilverHandEntrance'})}))
$actor=Block $source "struct ${identity}AI : ScriptedAI"
[IO.File]::WriteAllText("$out/GossipSelect.inc",$(if($Baseline){''}else{Block $actor 'void sGossipSelect('}))
$script=Block $source "class $identity : public CreatureScript"
if(!$Baseline -and $script.Contains('bool OnGossipSelect(')){throw 'Legacy unvalidated script callback remains'}
[IO.File]::WriteAllText("$out/LegacyGossip.inc",$(if($Baseline){Block $script 'bool OnGossipSelect('}else{''}))
[IO.File]::WriteAllText("$out/PacketNative.inc",(Block (Get-Content "$repo/src/server/game/Handlers/NPCHandler.cpp" -Raw) 'void WorldSession::HandleGossipSelectOption('))
$native=Get-Content "$repo/src/server/game/Entities/Creature/GossipDef.cpp" -Raw
$blocks=@('GossipMenuItem const* GossipMenu::GetItem(','uint32 GossipMenu::GetMenuItemSender(','uint32 GossipMenu::GetMenuItemAction(','void GossipMenu::ClearMenu(','GossipMenu& PlayerMenu::GetGossipMenu(','void PlayerMenu::ClearMenus(','uint32 PlayerMenu::GetGossipOptionSender(','uint32 PlayerMenu::GetGossipOptionAction(')
[IO.File]::WriteAllText("$out/MenuNative.inc",(($blocks|ForEach-Object{Block $native $_}) -join "`n"))
$containers=Get-Content "$repo/src/server/shared/Containers.h" -Raw
[IO.File]::WriteAllText("$out/MapLookup.inc",("namespace Trinity {template<class T>"+(Block $containers 'constexpr T* AddressOrSelf(T* ptr)')+"`ntemplate<class T>"+(Block $containers 'constexpr T* AddressOrSelf(T& not_ptr)')+"`nnamespace Containers {template<class M>"+(Block $containers 'auto MapGetValuePtr(')+"}}"))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($Baseline){'/DBASELINE'}else{''}
if($Second){$mode+=' /DSECOND_TRAVARD'}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $mode /I`"$out`" `"$PSScriptRoot/SilverHandGossipTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Silver Hand gossip regression failed'}
if($Baseline){return}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SilverHandPickupCatalog.cpp -ProbeName SilverHandPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native entrance metadata failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$criteria=if($Second){@("CRITERION`t4`t30079`t92`t50897`t0")}else{@("CRITERION`t0`t29843`t92`t50765`t0","CRITERION`t1`t30017`t92`t50766`t0")}
foreach($expected in $criteria){if((Get-Content "$out/native.tsv") -notcontains $expected){throw "Native stage changed: $expected"}}
if($VerifyReleaseBinary){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 $menu=if($Second){19599}else{19580};$pathId=if($Second){10993806}else{10993804}
 $query="SET SESSION TRANSACTION READ ONLY;SELECT 'binding',entry,gossip_menu_id,npcflag,npcflag2,ScriptName FROM creature_template WHERE ScriptName='$identity';SELECT 'gossip',MenuID,OptionID,OptionNpc,ActionMenuID,ActionPoiID,BoxCoded,BoxMoney FROM gossip_menu_option WHERE MenuID=$menu;SELECT 'path',id,COUNT(*),MIN(point),MAX(point) FROM waypoint_data_script WHERE id=$pathId GROUP BY id;"
 if($Second){$query+="SELECT 'spawn',guid,id,map,PhaseId FROM creature WHERE id=106639;SELECT 'phase',pd.phaseId,co.ConditionTypeOrReference,co.ConditionValue1,co.ConditionValue2,co.NegativeCondition FROM phase_definitions pd JOIN conditions co ON co.SourceGroup=pd.zoneId AND co.SourceEntry=pd.entry AND co.SourceTypeOrReferenceId=23 WHERE pd.zoneId=85 AND pd.entry=7;SELECT 'conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId IN(14,15) AND SourceGroup=19599;"}
 try{$env:MYSQL_PWD=$connection[3];$live=@($query|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Gossip binding query failed'}}finally{$env:MYSQL_PWD=$previous}
 $expected=if($Second){@("binding`t106639`t19599`t0`t0`tnpc_travar_second","gossip`t19599`t0`t0`t0`t0`t0`t0","path`t10993806`t15`t1`t15","spawn`t361991`t106639`t1539`t7000","phase`t7000`t44`t1092`t4`t0","conditions`t0")}else{@("binding`t106429`t19580`t0`t0`tnpc_travar_first","gossip`t19580`t0`t0`t0`t0`t0`t0","path`t10993804`t11`t1`t11")}
 if(Compare-Object $expected $live){throw 'Native Travard menu/path/binding changed'}
 $previous=$env:PYTHONPATH
 $verifier=if($Second){'verify_silver_hand_second_gossip_release.py'}else{'verify_silver_hand_gossip_release.py'}
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/$verifier" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed gossip guard missing'}}finally{$env:PYTHONPATH=$previous}
}
