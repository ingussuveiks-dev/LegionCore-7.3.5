param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-conquest"
New-Item -ItemType Directory -Force $out | Out-Null
function Query([string]$sql,[string]$database='World'){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match "^${database}DatabaseInfo\s*="} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Conquest SQL check failed'}}finally{$env:MYSQL_PWD=$previous}
 $rows
}
$migration=Get-Content "$repo/sql/updates/world/2026_10_18_01_stormheim_conquest.sql" -Raw
$snapshot=@'
SELECT 'sai',entryorguid,source_type,id,link,event_type,event_flags,event_param1,event_param2,action_type,action_param1,action_param2,action_param3,target_type FROM smart_scripts ORDER BY entryorguid,source_type,id;
SELECT 'condition',ConditionTypeOrReference,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions ORDER BY ConditionTypeOrReference,ConditionValue1,ConditionValue2;
SELECT 'eo',entry,ScriptName,radius FROM eventobject_template;
SELECT 'ai',entry,AIName,ScriptName FROM creature_template;
SELECT 'unlock-display',ID,RewardDisplaySpell1,RewardSpell FROM quest_template;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_sai AS SELECT * FROM smart_scripts WHERE entryorguid IN(373,92539,9253900,9253901,114957,115254,11525400,115263,115291);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_sai; INSERT INTO smart_scripts SELECT * FROM saved_sai;
CREATE TEMPORARY TABLE conditions AS SELECT * FROM conditions WHERE (SourceTypeOrReferenceId=15 AND SourceGroup=20539 AND SourceEntry=1) OR (SourceTypeOrReferenceId=22 AND SourceEntry=373 AND SourceId IN(10,13));
CREATE TEMPORARY TABLE eventobject_template AS SELECT * FROM eventobject_template WHERE entry=373;
CREATE TEMPORARY TABLE creature_template AS SELECT * FROM creature_template WHERE entry=92539;
CREATE TEMPORARY TABLE quest_template AS SELECT * FROM quest_template WHERE ID=44721;
$migration
SELECT 'FIRST';
$snapshot
$migration
SELECT 'SECOND';
$snapshot
SELECT 'END';
SELECT 'old-eo',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=373;
SELECT 'early-credit',COUNT(*) FROM smart_scripts WHERE entryorguid=9253901 AND action_type IN(33,85);
SELECT 'other-sai-changes',COUNT(*) FROM saved_sai a LEFT JOIN smart_scripts b ON a.entryorguid=b.entryorguid AND a.source_type=b.source_type AND a.id=b.id WHERE a.entryorguid IN(92539,9253900,115263,115291) AND (b.id IS NULL OR a.action_type<>b.action_type OR a.action_param1<>b.action_param1 OR a.event_type<>b.event_type OR a.link<>b.link);
"@
$rows=@(Query $sql);$rows | Set-Content "$out/replay.tsv"
$a=[Array]::IndexOf($rows,'FIRST');$b=[Array]::IndexOf($rows,'SECOND');$e=[Array]::IndexOf($rows,'END')
$first=$rows[($a+1)..($b-1)] -join "`n";$second=$rows[($b+1)..($e-1)] -join "`n"
if($a -lt 0 -or $b -le $a -or $e -le $b -or $first -ne $second){throw 'Conquest migration is not stable on repeat'}
foreach($expected in @("unlock-display`t44721`t230408`t0","old-eo`t0","early-credit`t0","other-sai-changes`t0","condition`t9`t44721`t0`t0`t0","condition`t41`t44721`t114964`t1`t0","condition`t41`t44721`t115291`t4`t0","condition`t41`t44721`t114957`t1`t0","condition`t41`t44721`t114972`t1`t1","eo`t373`teventobject_stormheim_conquest`t100","ai`t92539`tSmartAI`tnpc_havi_conquest","sai`t114957`t0`t1`t0`t6`t0`t0`t0`t1`t0`t0`t0`t1","sai`t11525400`t9`t2`t0`t0`t0`t1500`t1500`t12`t115263`t3`t20000`t1")){
 if($rows -notcontains $expected){throw "Conquest migration mismatch: $expected"}
}
$live=@(Query @'
SELECT 'objective',Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID=44721 ORDER BY StorageIndex;
SELECT 'spawn',id,COUNT(*),MIN(map),MIN(PhaseId) FROM creature WHERE id IN(114957,115254,115291) GROUP BY id;
SELECT 'entry',guid,id,map,position_x,position_y,position_z FROM eventobject WHERE id=373;
SELECT 'phase',ConditionTypeOrReference,ConditionValue1 FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=169;
SELECT 'chain',ID,PrevQuestID,NextQuestID FROM quest_template_addon WHERE ID IN(44721,44729);
SELECT 'relation',id,quest FROM creature_questender WHERE quest=44721;
SELECT 'reward',RewardItem1,RewardAmount1,RewardSpell FROM quest_template WHERE ID=44721;
SELECT 'transform',Entry,Displayid1 FROM creature_template_wdb WHERE Entry=114982;
'@);$live | Set-Content "$out/world.tsv"
if($live -notcontains "transform`t114982`t64975"){throw 'Native Odyn transform model unavailable'}
foreach($expected in @("objective`t0`t114964`t1`t0`t0","objective`t0`t115291`t4`t1`t2","objective`t0`t114957`t1`t2`t0","objective`t0`t114972`t1`t3`t2","spawn`t114957`t1`t1220`t7511","spawn`t115254`t1`t1220`t7511","spawn`t115291`t10`t1220`t7511","entry`t410124`t373`t1220`t3452.99`t1748.27`t0.09295","phase`t9`t44721","chain`t44721`t44771`t44729","chain`t44729`t44721`t44868","relation`t92539`t44721","reward`t142535`t1`t0")){
 if($live -notcontains $expected){throw "Conquest content mismatch: $expected"}
}
function Extract([string]$source,[string]$start,[string]$end,[string]$name){$a=$source.IndexOf($start);$b=$source.IndexOf($end,$a);if($a -lt 0 -or $b -le $a){throw "Conquest boundary missing: $name"};[IO.File]::WriteAllText("$out/$name.inc",$source.Substring($a,$b-$a))}
$hotfix=@(Query "SELECT 'condition',COUNT(*) FROM player_condition WHERE ID IN(45693,47902,47903); SELECT 'task',COUNT(*) FROM quest_v2_cli_task WHERE ID IN(44737,44746,44923); SELECT 'display',COUNT(*) FROM creature_display_info WHERE ID IN(73303,70110,66121,64200,64975,64593);" 'Hotfix');$hotfix | Set-Content "$out/hotfix.tsv"
foreach($kind in @('condition','task','display')){if($hotfix -notcontains "$kind`t0"){throw "Inspect $kind overlay before trusting native-only catalog"}}
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
Extract $source 'bool PlayerConditionLogic(' 'bool ConditionMgr::IsPlayerMeetingCondition(' 'ConquestConditionLogic'
$source=Get-Content "$repo/src/server/scripts/Legion/zone_stormheim.cpp" -Raw
Extract $source 'namespace StormheimConquest' '// A Threat Rises (44771)' 'ConquestProduction'
$source=Get-Content "$repo/src/server/game/Entities/EventObject/EventObject.cpp" -Raw
Extract $source 'void EventObject::Update(' 'void EventObject::MoveInLineOfSight(' 'ConquestEventUpdate'
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
Extract $source 'void Player::AddQuestDelayedEvent(' 'void Player::BindQuestObject(' 'ConquestQuestDelay'
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
Extract $source '        case SMART_ACTION_CAST:' '        case SMART_ACTION_CAST_CUSTOM:' 'ConquestCast'
$a=$source.IndexOf('                    e.enableTimed = false;');$first=$source.IndexOf('{',$source.IndexOf('for (auto& i : mTimedActionList)',$a));$depth=1;$last=$first+1
while($depth -and $last -lt $source.Length){if($source[$last] -eq '{'){$depth++};if($source[$last] -eq '}'){$depth--};$last++}
if($a -lt 0 -or $depth){throw 'Native timed-list advance missing'}
[IO.File]::WriteAllText("$out/ConquestAdvance.inc",$source.Substring($a,$last-$a))
$source=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
Extract $source 'enum SmartCastFlags' '// one line in DB' 'ConquestCastFlags'
& "$PSScriptRoot/Test-StormheimCall.ps1" *> "$out/upstream.log"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-call/*.inc" | Copy-Item -Destination $out
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$release.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')
$libs=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($name in @('StormheimConquestCatalog','StormheimConquestTest','StormheimConquestSmartTest','StormheimConquestCreditTest')){
 $arguments=@('/nologo','/EHsc','/std:c++20','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/DBOOST_ALL_NO_LIB')
 $arguments+=($includes | ForEach-Object {'/I"'+$_+'"'});$arguments+='/I"'+$out+'"';$arguments+='"'+$PSScriptRoot+'/'+$name+'.cpp"';$arguments+='/Fe"'+$out+'/'+$name+'.exe"';$arguments+='/Fo"'+$out+'/'+$name+'.obj"'
 if($name -like '*Catalog'){$arguments+='/link';$arguments+=($libs | ForEach-Object {'"'+$_+'"'})}
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($arguments -join ' ')+"`nexit /b %errorlevel%`n");& "$out/build.cmd"
 if($LASTEXITCODE){throw "Conquest compilation failed: $name"}
}
$previous=$env:PATH
try{$env:PATH="$repo/build-extractors/bin/Release;$env:PATH";foreach($name in @('StormheimConquestCatalog','StormheimConquestTest','StormheimConquestSmartTest','StormheimConquestCreditTest')){
 if($name -like '*Catalog'){$result=@(& "$out/$name.exe" "$repo/build-extractors/bin/Release/dbc/enUS/")}else{$result=@(& "$out/$name.exe")};$code=$LASTEXITCODE;$result | Set-Content "$out/$name.log";$result
 if($code){throw "Conquest native check failed: $name/$code"}
}}finally{$env:PATH=$previous}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe";$ascii=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
 foreach($name in @('eventobject_stormheim_conquest','npc_havi_conquest')){if(!$ascii.Contains($name)){throw "Canonical Release lacks $name"}}
 foreach($name in @('scripts','game')){
  $relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match "$name\\Release\\$name.lib$"} | Select-Object -First 1
  if(!$relative){throw "Missing actual $name final-link input"}
  $lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
  if(!(Test-Path $lib) -or (Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw "Canonical Release predates actual $name library"}
 }
 'PASS: canonical Release registrations and actual scripts/game final-link inputs. Detailed PE call checks are in the deployment audit.'
}
'PASS: native event/quest/cast regressions, all four objective records and idempotent scoped migration. Client verification pending.'
