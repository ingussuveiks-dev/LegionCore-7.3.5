$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warlock-troops"
New-Item -ItemType Directory -Force $out | Out-Null
function Block([string]$text,[string]$marker){$a=$text.IndexOf($marker);if($a -lt 0){throw "Missing $marker"};$end=$text.IndexOf('{',$a)+1;$depth=1;while($depth -and $end -lt $text.Length){if($text[$end] -eq '{'){$depth++};if($text[$end] -eq '}'){$depth--};$end++};if($depth){throw "Unbalanced $marker"};$text.Substring($a,$end-$a)}
function Save($name,$value){[IO.File]::WriteAllText("$out/$name.inc",$value)}
$g=Get-Content "$repo/src/server/game/Garrison/Garrison.cpp" -Raw
$m=Get-Content "$repo/src/server/game/Garrison/GarrisonMgr.cpp" -Raw
Save 'Select' (Block $m 'uint32 GarrShipment::selectShipment(')
Save 'Place' (Block $g 'uint64 Garrison::PlaceShipment(')
Save 'Create' (Block $g 'void Garrison::CreateGarrisonShipment(')
Save 'Free' (Block $g 'void Garrison::FreeShipmentChest(')
$collect=Block $g 'void Garrison::CompleteShipments('
$end=$collect.IndexOf('    loot->shipmentBuildingType')
if($end -lt 0){throw 'Missing native item-free collection path'}
# These three native troop records have DummyItemID=0. Execute the entire
# production collection path up to its empty-item early return, including casts,
# follower creation and native queue cleanup. An item-bearing path fails here.
Save 'Collect' ($collect.Substring(0,$end)+"    assert(false);`n}")
$spells=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
Save 'EffectCreate' (Block $spells 'void Spell::EffectCreateGarrisonShipment(')
Save 'EffectCredit' (Block $spells 'void Spell::EffectKillCreditPersonal(')
$player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
Save 'Check' (Block $player 'bool Player::CheckShipment(')
$defs=Get-Content "$repo/src/server/game/Quests/QuestDef.h" -Raw
Save 'Status' ((Block $defs 'enum QuestStatus')+';')
$a=$g.IndexOf('    for (auto data : _shipments)',$g.IndexOf('void Garrison::Update('))
$b=$g.IndexOf('    if (GameTime::GetGameTime() > _MissionGen)',$a)
if($a -lt 0 -or $b -lt $a){throw 'Missing native shipment-ready update'}
Save 'Update' ("void Garrison::UpdateReady(){`n"+$g.Substring($a,$b-$a)+"}`n")
& "$PSScriptRoot/../research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarlockTroopCatalog.cpp -ProbeName WarlockTroopCatalog -OutputFile "$out/native.tsv"
[IO.File]::WriteAllLines("$out/native.tsv",[string[]](Get-Content "$out/native.tsv" -Encoding UTF8))
$migration=Get-Content "$repo/sql/updates/world/2026_10_21_04_dyala_gossip.sql" -Raw
$sql=@"
CREATE TEMPORARY TABLE saved_options LIKE gossip_menu_option;
INSERT INTO saved_options SELECT * FROM gossip_menu_option;
CREATE TEMPORARY TABLE gossip_menu_option LIKE saved_options;
INSERT INTO gossip_menu_option SELECT * FROM saved_options;
REPLACE INTO gossip_menu_option SELECT MenuID,1,OptionNpc,OptionText,OptionBroadcastTextID,ActionMenuID,ActionPoiID,BoxCoded,BoxMoney,BoxText,BoxBroadcastTextID,VerifiedBuild FROM saved_options WHERE MenuID=19918 AND OptionID=0;
REPLACE INTO gossip_menu_option SELECT MenuID,2,OptionNpc,OptionText,OptionBroadcastTextID,ActionMenuID,ActionPoiID,BoxCoded,BoxMoney,BoxText,BoxBroadcastTextID,VerifiedBuild FROM saved_options WHERE MenuID=19918 AND OptionID=0;
$migration
$migration
SELECT 'MENU',COUNT(*) FROM gossip_menu_option WHERE MenuID=19918;
SELECT 'KEPT',COUNT(*) FROM gossip_menu_option WHERE MenuID=19918 AND OptionID=0 AND OptionNpc=28 AND OptionBroadcastTextID=115147;
SELECT 'OTHER',COUNT(*) FROM saved_options a LEFT JOIN gossip_menu_option b ON a.MenuID=b.MenuID AND a.OptionID=b.OptionID WHERE a.MenuID<>19918 AND b.MenuID IS NULL;
-- An unrelated payload reusing a removed ID must survive the guarded delete.
INSERT INTO gossip_menu_option SELECT MenuID,2,OptionNpc,OptionText,0,ActionMenuID,ActionPoiID,BoxCoded,BoxMoney,BoxText,BoxBroadcastTextID,VerifiedBuild FROM saved_options WHERE MenuID=19918 AND OptionID=0;
$migration
SELECT 'DISTINCT',COUNT(*) FROM gossip_menu_option WHERE MenuID=19918 AND OptionID=2 AND OptionBroadcastTextID=0;
SELECT 'OBJECT',COUNT(*) FROM gameobject_template t JOIN gameobject g ON g.id=t.entry WHERE t.entry=250912 AND t.type=45 AND t.Data0=129 AND g.map=1107 AND g.phaseId='6930';
SELECT 'CREDIT',COUNT(*) FROM quest_objectives WHERE QuestID=41797 AND Type=0 AND ObjectID=106208 AND Amount=1;
"@
$db=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$prior=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$db[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($db[0]) --port=$($db[1]) --user=$($db[2]) --database=$($db[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Temporary troop menu fixture failed'}}finally{$env:MYSQL_PWD=$prior}
foreach($expected in @("MENU`t1","KEPT`t1","OTHER`t0","DISTINCT`t1","OBJECT`t1","CREDIT`t1")){if($rows -notcontains $expected){throw "Troop world fixture mismatch: $expected"}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/WarlockTroopsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/native.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native troop lifecycle regression failed'}
