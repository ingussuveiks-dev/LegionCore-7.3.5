param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Pinned gossip baseline cannot verify installed code'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hruthnir-gossip"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/EarthWarder.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "6e673f47:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Hruthnir source unavailable'}}
$source=$source.Substring(0,$source.IndexOf('class npc_magnar_icebreaker'))
[IO.File]::WriteAllText("$out/Policy.inc",$(if($Baseline){''}else{Block $source 'namespace HruthnirGossip'}))
[IO.File]::WriteAllText("$out/Select.inc",(Block $source 'bool OnGossipSelect('))
[IO.File]::WriteAllText("$out/Hello.inc",$(if($Baseline){'bool OnGossipHello(Player*, Creature*) override { return false; }'}else{Block $source 'bool OnGossipHello('}))
[IO.File]::WriteAllText("$out/AISelect.inc",$(if($Baseline){''}else{Block $source 'void sGossipSelect('}))
$native=Get-Content "$repo/src/server/game/Handlers/NPCHandler.cpp" -Raw
[IO.File]::WriteAllText("$out/Dispatch.inc",(Block $native 'void WorldSession::HandleGossipSelectOption('))
$native=Get-Content "$repo/src/server/game/Entities/Creature/GossipDef.cpp" -Raw
$methods=@('uint32 GossipMenu::GetMenuItemSender(','uint32 GossipMenu::GetMenuItemAction(','GossipMenuItem const* GossipMenu::GetItem(','uint32 GossipMenu::GetMenuId(','void GossipMenu::ClearMenu(')
[IO.File]::WriteAllText("$out/Menu.inc",(($methods|ForEach-Object {Block $native $_}) -join "`n"))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/HruthnirGossipTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Extracted Hruthnir gossip regression failed'}
if($VerifyReleaseBinary){
 & "$PSScriptRoot/Test-MagnarWaves.ps1" -VerifyReleaseBinary *> "$out/magnar.log"
 if(!$? -or $LASTEXITCODE){throw "Magnar regressions failed: $out/magnar.log"}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=$connection[3]
  $rows=@(@'
SELECT 'actor',entry,gossip_menu_id,npcflag,ScriptName FROM creature_template WHERE entry=96468;
SELECT 'menu',MenuID,TextID FROM gossip_menu WHERE MenuID=18743;
SELECT 'option',MenuID,OptionID,OptionNpc,OptionBroadcastTextID,ActionMenuID,ActionPoiID,BoxCoded,BoxMoney FROM gossip_menu_option WHERE MenuID=18743;
SELECT 'conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId IN(14,15) AND SourceGroup=18743;
'@|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
  if($LASTEXITCODE){throw 'Read-only Hruthnir menu metadata failed'}
 }finally{$env:MYSQL_PWD=$previous}
 [IO.File]::WriteAllLines("$out/menu.tsv",[string[]]$rows)
 $expected=@("actor`t96468`t18743`t0`tnpc_hrutnir","menu`t18743`t27242","option`t18743`t0`t0`t98793`t0`t0`t0`t0","conditions`t0")
 if(Compare-Object $expected $rows){throw 'Native Hruthnir menu mapping differs'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_hruthnir_gossip_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Hruthnir policy missing'}}finally{$env:PYTHONPATH=$previous}
}
