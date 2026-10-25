param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned baseline cannot verify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/silver-hand-gossip"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/SilverHand/SilverHand.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "357347d7:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned gossip baseline unavailable'}}
[IO.File]::WriteAllText("$out/EntrancePolicy.inc",(Block $source 'namespace SilverHandEntrance'))
$actor=Block $source 'struct npc_travar_firstAI : ScriptedAI'
[IO.File]::WriteAllText("$out/GossipSelect.inc",$(if($Baseline){''}else{Block $actor 'void sGossipSelect('}))
$script=Block $source 'class npc_travar_first : public CreatureScript'
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
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $mode /I`"$out`" `"$PSScriptRoot/SilverHandGossipTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Silver Hand gossip regression failed'}
if($Baseline){return}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SilverHandPickupCatalog.cpp -ProbeName SilverHandPickupCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native entrance metadata failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
foreach($expected in @("CRITERION`t0`t29843`t92`t50765`t0","CRITERION`t1`t30017`t92`t50766`t0")){if((Get-Content "$out/native.tsv") -notcontains $expected){throw "Native stage changed: $expected"}}
if($VerifyReleaseBinary){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$live=@("SET SESSION TRANSACTION READ ONLY;SELECT 'binding',entry,gossip_menu_id,npcflag,npcflag2,ScriptName FROM creature_template WHERE ScriptName='npc_travar_first';SELECT 'gossip',MenuID,OptionID,OptionNpc,ActionMenuID,ActionPoiID,BoxCoded,BoxMoney FROM gossip_menu_option WHERE MenuID=19580;SELECT 'path',id,COUNT(*),MIN(point),MAX(point) FROM waypoint_data_script WHERE id=10993804 GROUP BY id;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Gossip binding query failed'}}finally{$env:MYSQL_PWD=$previous}
 $expected=@("binding`t106429`t19580`t0`t0`tnpc_travar_first","gossip`t19580`t0`t0`t0`t0`t0`t0","path`t10993804`t11`t1`t11")
 if(Compare-Object $expected $live){throw 'Native Travard menu/path/binding changed'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_silver_hand_gossip_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed gossip guard missing'}}finally{$env:PYTHONPATH=$previous}
}
