param([switch]$Baseline,[switch]$CueBaseline,[switch]$OfferBaseline,[switch]$StageBaseline,[switch]$VoiceBaseline,[switch]$ArrivalBaseline,[switch]$OfferArrivalBaseline,[switch]$HomeBaseline,[switch]$RequestBaseline,[switch]$ReadyBaseline,[switch]$ConversationBaseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if(($Baseline -or $CueBaseline -or $OfferBaseline -or $StageBaseline -or $VoiceBaseline -or $ArrivalBaseline -or $OfferArrivalBaseline -or $HomeBaseline -or $RequestBaseline -or $ReadyBaseline -or $ConversationBaseline) -and $VerifyReleaseBinary){throw 'Pinned gossip baseline cannot verify installed code'}
if(@($Baseline,$CueBaseline,$OfferBaseline,$StageBaseline,$VoiceBaseline,$ArrivalBaseline,$OfferArrivalBaseline,$HomeBaseline,$RequestBaseline,$ReadyBaseline,$ConversationBaseline | Where-Object {$_}).Count -gt 1){throw 'Select only one pinned baseline'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hruthnir-gossip"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/EarthWarder.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "6e673f47:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Hruthnir source unavailable'}}
if($CueBaseline){$source=(& git -C $repo show "05d64caf:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Hruthnir cue source unavailable'}}
if($OfferBaseline){$source=(& git -C $repo show "bbf4455c:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Hruthnir offer source unavailable'}}
if($StageBaseline){$source=(& git -C $repo show "c4c026d1:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Hruthnir stage source unavailable'}}
if($VoiceBaseline){$source=(& git -C $repo show "c7239252:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Hruthnir voice source unavailable'}}
if($ArrivalBaseline){$source=(& git -C $repo show "31d576d9:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Hruthnir arrival source unavailable'}}
if($OfferArrivalBaseline){$source=(& git -C $repo show "9a0e4426:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Hruthnir offer arrival source unavailable'}}
if($HomeBaseline){$source=(& git -C $repo show "5e944704:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Hruthnir home source unavailable'}}
if($RequestBaseline){$source=(& git -C $repo show "3d445173:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Hruthnir request source unavailable'}}
if($ReadyBaseline){$source=(& git -C $repo show "da79baae:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Hruthnir readiness source unavailable'}}
if($ConversationBaseline){$source=(& git -C $repo show "688c16b9:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Hruthnir conversation source unavailable'}}
$source=$source.Substring(0,$source.IndexOf('class npc_magnar_icebreaker'))
[IO.File]::WriteAllText("$out/Policy.inc",$(if($Baseline){''}else{Block $source 'namespace HruthnirGossip'}))
[IO.File]::WriteAllText("$out/Select.inc",(Block $source 'bool OnGossipSelect('))
[IO.File]::WriteAllText("$out/Hello.inc",$(if($Baseline){'bool OnGossipHello(Player*, Creature*) override { return false; }'}else{Block $source 'bool OnGossipHello('}))
[IO.File]::WriteAllText("$out/AISelect.inc",$(if($Baseline){''}else{Block $source 'void sGossipSelect('}))
$first=$source.IndexOf('        EventMap events;');$last=$source.IndexOf($(if($Baseline){'        void Reset()'}else{'        void sGossipSelect('}),$first)
[IO.File]::WriteAllText("$out/AIFields.inc",$source.Substring($first,$last-$first))
[IO.File]::WriteAllText("$out/AIReset.inc",(Block $source 'void Reset()'))
[IO.File]::WriteAllText("$out/AIUpdate.inc",(Block $source 'void UpdateAI('))
[IO.File]::WriteAllText("$out/OfferContext.inc",$(if($source.Contains('bool IsMagnarOfferContext(')){Block $source 'bool IsMagnarOfferContext('}else{''}))
[IO.File]::WriteAllText("$out/OfferPosition.inc",$(if($source.Contains('bool IsMagnarOfferPosition(')){Block $source 'bool IsMagnarOfferPosition('}else{''}))
[IO.File]::WriteAllText("$out/Request.inc",$(if($source.Contains('bool HasMagnarChamberRequest(')){Block $source 'bool HasMagnarChamberRequest('}else{''}))
[IO.File]::WriteAllText("$out/Conversation.inc",$(if($source.Contains('bool StartMagnarConversation(')){Block $source 'bool StartMagnarConversation('}else{''}))
[IO.File]::WriteAllText("$out/UnitConversation.inc",(Block (Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw) 'void Unit::CreateConversation(').Replace('void Unit::','void Creature::'))
[IO.File]::WriteAllText("$out/ConversationRemove.inc",(Block (Get-Content "$repo/src/server/game/Entities/Conversation/Conversation.cpp" -Raw) 'void Conversation::Remove(').Replace('void Conversation::','void '))
$ready=''
if($source.Contains('bool IsMagnarOfferReady(')){
 $ready=(Block $source 'bool IsMagnarOfferReady(')+'uint32 GetData(uint32 type) const override {'+(Block (Block $source 'uint32 GetData(') 'if (type == 1061)')+'return 0;}'
}
[IO.File]::WriteAllText("$out/Ready.inc",$ready)
[IO.File]::WriteAllText("$out/IntroContext.inc",$(if($source.Contains('bool IsMagnarIntroContext(')){Block $source 'bool IsMagnarIntroContext('}else{''}))
[IO.File]::WriteAllText("$out/Chamber.inc",$(if($source.Contains('bool IsMagnarIntroContext(')){'void StartChamber(){switch(2){'+(Block (Block $source 'void DoAction(') 'case 2:')+'}}'}else{''}))
$movement=Block $source 'void MovementInform('
[IO.File]::WriteAllText("$out/Movement.inc",$movement)
$first=$movement.IndexOf('case 6:',$movement.IndexOf('if (type == POINT_MOTION_TYPE)'));$last=$movement.IndexOf('case 7:',$first)
if($first -lt 0 -or $last -le $first){throw 'Native point6 branch missing'}
[IO.File]::WriteAllText("$out/Offer.inc",'void OfferArrive() { switch(6) {'+$movement.Substring($first,$last-$first)+'} }')
$first=$movement.IndexOf('case 7:',$movement.IndexOf('if (type == POINT_MOTION_TYPE)'));$last=$movement.IndexOf('default:',$first)
if($first -lt 0 -or $last -le $first){throw 'Native point7 branch missing'}
[IO.File]::WriteAllText("$out/Arrival.inc",'void Arrive() { switch(7) {'+$movement.Substring($first,$last-$first)+'} }')
$native=Get-Content "$repo/src/server/game/Movement/MovementGenerators/PointMovementGenerator.cpp" -Raw
[IO.File]::WriteAllText("$out/PointFinalize.inc",'template<class T>'+(Block $native 'void PointMovementGenerator<T>::DoFinalize('))
[IO.File]::WriteAllText("$out/PointInform.inc",'template<>'+(Block $native 'void PointMovementGenerator<Creature>::MovementInform('))
$native=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw
[IO.File]::WriteAllText("$out/FunctionsClass.inc",'using FunctionList = std::multimap<uint64, std::function<void()>>;'+(Block $native 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';')
$native=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
[IO.File]::WriteAllText("$out/Functions.inc",$native.Substring($native.IndexOf('FunctionProcessor::FunctionProcessor()')))
$native=Get-Content "$repo/src/server/game/Entities/Unit/Unit.h" -Raw
[IO.File]::WriteAllText("$out/AddDelayed.inc",(Block $native 'void AddDelayedEvent('))
$native=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/SetHome.inc",(Block $native 'void Creature::SetHomePosition(float').Replace('void Creature::','void '))
$native=Get-Content "$repo/src/server/game/Handlers/NPCHandler.cpp" -Raw
[IO.File]::WriteAllText("$out/Dispatch.inc",(Block $native 'void WorldSession::HandleGossipSelectOption('))
$native=Get-Content "$repo/src/server/game/Entities/Creature/GossipDef.cpp" -Raw
$methods=@('uint32 GossipMenu::GetMenuItemSender(','uint32 GossipMenu::GetMenuItemAction(','GossipMenuItem const* GossipMenu::GetItem(','uint32 GossipMenu::GetMenuId(','void GossipMenu::ClearMenu(')
[IO.File]::WriteAllText("$out/Menu.inc",(($methods|ForEach-Object {Block $native $_}) -join "`n"))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}elseif($CueBaseline){'/DCUE_BASELINE'}elseif($OfferBaseline){'/DOFFER_BASELINE'}elseif($StageBaseline){'/DSTAGE_BASELINE'}elseif($VoiceBaseline){'/DVOICE_BASELINE'}elseif($ArrivalBaseline){'/DARRIVAL_BASELINE'}elseif($OfferArrivalBaseline){'/DOFFER_ARRIVAL_BASELINE'}elseif($HomeBaseline){'/DHOME_BASELINE'}elseif($RequestBaseline){'/DREQUEST_BASELINE'}else{''}
if($source.Contains('bool HasMagnarChamberRequest(')){$definition+=' /DCURRENT_REQUEST'}
if($ReadyBaseline){$definition+=' /DREADY_BASELINE'}
if($ConversationBaseline){$definition+=' /DCONVERSATION_BASELINE'}
if($source.Contains('bool IsMagnarOfferReady(')){$definition+=' /DCURRENT_READY'}
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
SELECT 'offer-voice',GroupID,ID,BroadcastTextID,Duration,Sound FROM creature_text WHERE CreatureID=96468 AND GroupID IN(15,16) ORDER BY GroupID,ID;
SELECT 'cue-line',id,idx,textId,unk2 FROM conversation_data WHERE entry=497 ORDER BY id;
SELECT 'cue-actor',id,creatureId,duration FROM conversation_creature WHERE entry=497 ORDER BY id;
SELECT 'cue-models',COUNT(*) FROM conversation_actor WHERE entry=497;
'@|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
  if($LASTEXITCODE){throw 'Read-only Hruthnir menu metadata failed'}
 }finally{$env:MYSQL_PWD=$previous}
 [IO.File]::WriteAllLines("$out/menu.tsv",[string[]]$rows)
 $expected=@("actor`t96468`t18743`t0`tnpc_hrutnir","menu`t18743`t27242","option`t18743`t0`t0`t98793`t0`t0`t0`t0","conditions`t0")
 $expected+=@("offer-voice`t15`t0`t98324`t0`t60403","offer-voice`t16`t0`t98325`t0`t60404")
 $expected+=@("cue-line`t1324`t0`t0`t0","cue-line`t1325`t1`t7000`t1","cue-line`t1326`t3`t12000`t0","cue-line`t1327`t4`t19000`t1","cue-line`t1328`t5`t22000`t0","cue-line`t1329`t6`t30000`t1","cue-line`t1330`t7`t32000`t0","cue-line`t1331`t2`t10000`t1","cue-line`t1332`t8`t40000`t1","cue-line`t1334`t9`t41000`t258","cue-actor`t0`t96468`t68000","cue-actor`t1`t96034`t0","cue-actor`t2`t0`t0","cue-models`t0")
 if(Compare-Object $expected $rows){throw 'Native Hruthnir menu mapping differs'}
 $native=Get-Content "$repo/build-extractors/runtime-test/magnar-waves/dialogue.tsv"
 foreach($expected in @("LINE`t1324`t98326`t0`t500`t1325`t0`t0`t60`t60","LINE`t1325`t98336`t0`t500`t1331`t0`t0`t60`t60","LINE`t1326`t98329`t0`t500`t1327`t0`t0`t60`t60","LINE`t1327`t98327`t0`t0`t1328`t0`t0`t60`t60","LINE`t1328`t98330`t0`t500`t1329`t0`t0`t60`t60","LINE`t1329`t98331`t0`t500`t1330`t0`t0`t60`t60","LINE`t1330`t98332`t0`t500`t1332`t0`t0`t60`t60","LINE`t1331`t98799`t0`t500`t1326`t0`t1`t60`t60","LINE`t1332`t98800`t0`t500`t1334`t0`t1`t60`t60","LINE`t1334`t0`t0`t0`t0`t0`t0`t60`t60","NONVERBAL`t1334","SOUND`t98326`t60405`t1370060","SOUND`t98336`t60407`t1370062","SOUND`t98329`t60409`t1370064","SOUND`t98327`t60410`t1370065","SOUND`t98330`t60411`t1370066","SOUND`t98331`t60412`t1370067","SOUND`t98332`t60413`t1370068","SOUND`t98799`t60408`t1370063","SOUND`t98800`t60414`t1370069")){if($native -notcontains $expected){throw "Native497 metadata differs: $expected"}}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_hruthnir_creation_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Hruthnir policy missing'}}finally{$env:PYTHONPATH=$previous}
}
