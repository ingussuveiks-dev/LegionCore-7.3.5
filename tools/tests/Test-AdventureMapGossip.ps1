param([switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/adventure-map-gossip"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$header=Get-Content "$repo/src/server/game/Entities/Creature/GossipDef.h" -Raw
$a=$header.IndexOf('enum class GossipOptionNpc');$b=$header.IndexOf('//POI icons',$a)
$enums=$header.Substring($a,$b-$a)
$header=Get-Content "$repo/src/server/game/Entities/Unit/UnitDefines.h" -Raw
$a=$header.IndexOf('enum NPCFlags :');$b=$header.IndexOf('enum MovementFlags',$a)
[IO.File]::WriteAllText("$out/Enums.inc",$enums+$header.Substring($a,$b-$a))
$source=Get-Content "$repo/src/server/game/Entities/Creature/GossipDef.cpp" -Raw
$a=$source.IndexOf('uint64 GossipMenu::GetRequiredNpcFlagForOption');$b=$source.IndexOf('PlayerMenu::PlayerMenu',$a)
[IO.File]::WriteAllText("$out/RequiredFlags.inc",$source.Substring($a,$b-$a))
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$filter=[regex]::Match($source,'            if \(!\(GossipMenu::GetRequiredNpcFlagForOption\(optionNpc\) & npcflags\)\)\s*continue;')
if(!$filter.Success){throw 'Production gossip admission changed; update this test'}
[IO.File]::WriteAllText("$out/Admission.inc",$filter.Value)
$flags=[regex]::Match($source,'        npcflags = uint64\(source->GetUInt32Value\(UNIT_FIELD_NPC_FLAGS\)\) \|\s*\(uint64\(source->GetUInt32Value\(UNIT_FIELD_NPC_FLAGS2\)\) << 32\);')
if(!$flags.Success){throw 'Production gossip flags must include both distinct NPC fields'}
[IO.File]::WriteAllText("$out/Flags.inc",$flags.Value)
function FunctionBlock([string]$text,[string]$marker){
 $a=$text.IndexOf($marker);if($a -lt 0){throw "Missing $marker"};$b=$text.IndexOf('{',$a);$end=$b+1;$depth=1
 while($depth -and $end -lt $text.Length){if($text[$end] -eq '{'){$depth++};if($text[$end] -eq '}'){$depth--};$end++}
 if($depth){throw "Unbalanced $marker"};$text.Substring($a,$end-$a)
}
$shared=Get-Content "$repo/src/server/game/Miscellaneous/SharedDefines.h" -Raw
[IO.File]::WriteAllText("$out/Classes.inc",(FunctionBlock $shared 'enum Classes')+';')
$qdefs=Get-Content "$repo/src/server/game/Quests/QuestDef.h" -Raw
[IO.File]::WriteAllText("$out/QuestStatus.inc",(FunctionBlock $qdefs 'enum QuestStatus')+';')
$garr=Get-Content "$repo/src/server/game/Garrison/Garrison.cpp" -Raw
foreach($pair in @(@('CanAddShipment','bool Garrison::canAddShipmentOrder('),@('CanStartUpgrade','bool Garrison::canStartUpgrade('),@('OpenShipment','void Garrison::OnGossipSelect('))){[IO.File]::WriteAllText("$out/$($pair[0]).inc",(FunctionBlock $garr $pair[1]))}
$garrMgr=Get-Content "$repo/src/server/game/Garrison/GarrisonMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/SelectShipment.inc",(FunctionBlock $garrMgr 'uint32 GarrShipment::selectShipment('))
$a=$source.IndexOf('                case GossipOptionNpc::ShipmentCrafter:');$b=$source.IndexOf('                case GossipOptionNpc::GarrisonTradeskill:',$a)
[IO.File]::WriteAllText("$out/ShipmentGate.inc",$source.Substring($a,$b-$a))
$a=$source.IndexOf('                case GossipOptionNpc::GarrisonTalent:');$b=$source.IndexOf('                case GossipOptionNpc::Trainer:',$a)
[IO.File]::WriteAllText("$out/TalentGate.inc",$source.Substring($a,$b-$a))
$a=$source.IndexOf('        case GossipOptionNpc::AdventureMap:',$source.IndexOf('void Player::OnGossipSelect'))
$b=$source.IndexOf('        case GossipOptionNpc::GarrisonTalent:',$a)
[IO.File]::WriteAllText("$out/Select.inc",$source.Substring($a,$b-$a))
$a=$source.IndexOf('uint16 Player::getAdventureQuestID()');$b=$source.IndexOf('int32 Player::GetCommandCooldown()',$a)
[IO.File]::WriteAllText("$out/Credit.inc",$source.Substring($a,$b-$a))
$source=Get-Content "$repo/src/server/game/Handlers/QuestHandler.cpp" -Raw
[IO.File]::WriteAllText("$out/JournalOpen.inc",(FunctionBlock $source 'void WorldSession::HandleAdventureJournalOpenQuest('))
$a=$source.IndexOf('void WorldSession::HandleAdventureJournalStartQuest(');$b=$source.IndexOf('void WorldSession::HandleQueryAdventureMapPOI(',$a)
[IO.File]::WriteAllText("$out/Adventure.inc",$source.Substring($a,$b-$a))
$sql=@"
SELECT t.entry,t.npcflag,t.npcflag2 FROM creature_template t JOIN gossip_menu_option o ON o.MenuID=t.gossip_menu_id WHERE o.OptionNpc=31 ORDER BY t.entry;
"@
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try {$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Scouting-map catalog read failed'}}finally{$env:MYSQL_PWD=$old}
if(!$rows.Count){throw 'No native scouting-map actors found'}
[IO.File]::WriteAllLines("$out/boards.tsv",$rows)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/AdventureMapGossipTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`" `"$out/boards.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Production adventure-map interaction regression failed'}
$serviceCmd=$cmd.Replace('AdventureMapGossipTest.cpp','GarrisonGossipServiceTest.cpp').Replace('`"$out/boards.tsv`"','')
[IO.File]::WriteAllText("$out/run-services.cmd",$serviceCmd);& "$out/run-services.cmd"
if($LASTEXITCODE){throw 'Production class-hall service regression failed'}
if($VerifyReleaseBinary){
 if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
 $binary=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes("$ReleaseDirectory/worldserver.exe"))
 if(!$binary.Contains('AdventureMap: opening scouting map for player %u at creature %u')){throw 'Canonical Release lacks adventure-map interaction repair'}
 $python=Get-Command python -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -First 1
 if(!$python){$python='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'}
 & $python "$PSScriptRoot/verify_gossip_service_release.py" $ReleaseDirectory
 if($LASTEXITCODE){throw 'Actual Release gossip service lookup is stale'}
}
'PASS: actual gossip filter on every scouting-map template, map packet dispatch, native availability/start handlers, exactly-once selection and retry boundaries. Client map pins pending.'
