param([switch]$Baseline,[switch]$AddressSanitizer,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot verify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/ashbringer-pickup-admission"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/broken_shore.cpp'
$source=Get-Content "$repo/$path" -Raw
$pinned=(& git -C $repo show "30be60aa:$path") -join "`n"
if($LASTEXITCODE){throw 'Pinned admission baseline unavailable'}
$source=$source.Replace((Block $source 'class spell_bs_dominate_mind :'),(Block $pinned 'class spell_bs_dominate_mind :'))
$helper=Block $source 'namespace AshbringerAcquisition'
$current=Block $source 'class go_bs_the_ashbringer :'
$restored=$current.Replace((Block $current 'struct AI : GameObjectAI')+';','').Replace((Block $current 'bool OnGossipHello('),'').Replace('GameObjectAI* GetAI(GameObject* go) const override { return new AI(go); }','') -replace 'if \(!AshbringerAcquisition::CanRetrieve\(player, go\)\)\s*return;',''
if(($restored -replace '\s','') -cne ((Block $pinned 'class go_bs_the_ashbringer :') -replace '\s','')){throw 'Existing pickup behavior changed beyond admission hooks'}
$outside=$source.Replace($helper,'').Replace($current,(Block $pinned 'class go_bs_the_ashbringer :'))
foreach($header in @('GameObjectAI.h','Scenario.h','ScenarioMgr.h')){$outside=$outside.Replace("#include `"$header`"",'')}
if(($outside -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Unrelated scenario behavior changed'}
if($Baseline){$current=Block $pinned 'class go_bs_the_ashbringer :';$helper=''}
[IO.File]::WriteAllText("$out/Pickup.inc",$helper+$current+';')
$constants=(Block (Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Paladin/BrokenShore/broken_shore.h" -Raw) 'enum Data')+';'+(Block $source 'enum eSpells')+';'
foreach($pair in @(@('Entities/GameObject/GameObject.h','enum LootState'),@('Entities/Unit/Unit.h','enum InventorySlot'),@('Entities/Item/Item.h','enum InventoryResult'),@('Miscellaneous/SharedDefines.h','enum SpellCastResult'),@('Quests/QuestDef.h','enum QuestStatus'),@('Miscellaneous/SharedDefines.h','enum Classes'),@('Miscellaneous/SharedDefines.h','enum GameObjectFlags'))){$constants+=(Block (Get-Content "$repo/src/server/game/$($pair[0])" -Raw) $pair[1])+';'}
[IO.File]::WriteAllText("$out/Constants.inc",$constants)
[IO.File]::WriteAllText("$out/CapacityBridge.inc",(Block (Get-Content "$repo/src/server/game/Entities/Player/Player.h" -Raw) 'InventoryResult CanStoreNewItem('))
$native=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
$first=$native.IndexOf('void GameObject::SetLootState(');$last=$native.IndexOf('    if (m_model)',$first)
if($first -lt 0 -or $last -le $first){throw 'Native state dispatcher missing'}
[IO.File]::WriteAllText("$out/SetLootState.inc",$native.Substring($first,$last-$first)+'}')
$effects=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
if($effects -notmatch 'sScriptMgr->OnGossipHello\(player, gameObjTarget\)' -or $effects -notmatch 'gameObjTarget->AI\(\)->GossipHello\(player, true\)'){throw 'Native spell-open admission changed'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=$(if($Baseline){'/DBASELINE '}else{''})+$(if($AddressSanitizer){'/fsanitize=address /Zi'}else{''})
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/AshbringerPickupAdmissionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native pickup admission regression failed'}
if($VerifyReleaseBinary){$previous=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_ashbringer_pickup_admission_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Installed admission guard missing'}}finally{$env:PYTHONPATH=$previous}}
