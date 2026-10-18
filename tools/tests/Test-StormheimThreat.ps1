param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-threat"
New-Item -ItemType Directory -Force $out | Out-Null
$migration=Get-Content "$repo/sql/updates/world/2026_10_18_00_stormheim_threat.sql" -Raw
function Query([string]$database,[string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match "^${database}DatabaseInfo\s*="} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw ('Threat data/replay query failed: '+($rows -join "`n"))}}finally{$env:MYSQL_PWD=$previous}
 @($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")})
}
$snapshot=@'
SELECT 'sai',entryorguid,source_type,id,link,event_type,event_param1,event_param2,action_type,action_param1,action_param2,target_type FROM smart_scripts ORDER BY entryorguid,source_type,id;
SELECT 'condition',SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,NegativeCondition FROM conditions ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionValue1;
'@
$sql=@"
CREATE TEMPORARY TABLE saved_sai AS SELECT * FROM smart_scripts WHERE entryorguid IN(92539,9253900,9253901);
CREATE TEMPORARY TABLE smart_scripts LIKE saved_sai;
INSERT INTO smart_scripts SELECT * FROM saved_sai;
CREATE TEMPORARY TABLE conditions AS SELECT * FROM conditions WHERE (SourceTypeOrReferenceId=23 AND SourceGroup=7541 AND SourceEntry=168) OR (SourceTypeOrReferenceId=17 AND SourceEntry IN(229547,229539,229554));
-- Reproduce the old timer and bad AND even when testing an already migrated DB.
UPDATE conditions SET ElseGroup=0 WHERE SourceTypeOrReferenceId=23 AND ConditionValue1=42004;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry IN(229547,229539);
$migration
SELECT 'FIRST';
$snapshot
$migration
SELECT 'SECOND';
$snapshot
SELECT 'END';
SELECT 'preserved-sai',COUNT(*) FROM saved_sai a LEFT JOIN smart_scripts b ON a.entryorguid=b.entryorguid AND a.source_type=b.source_type AND a.id=b.id WHERE NOT(a.entryorguid=9253900 AND a.source_type=9 AND a.id IN(8,9)) AND (b.id IS NULL OR a.action_type<>b.action_type OR a.action_param1<>b.action_param1 OR a.event_param1<>b.event_param1 OR a.event_param2<>b.event_param2 OR a.target_type<>b.target_type OR a.link<>b.link);
SELECT 'no-timer-credit',COUNT(*) FROM smart_scripts WHERE entryorguid=9253900 AND action_type IN(33,206);
"@
[IO.File]::WriteAllText("$out/replay.sql",$sql);$rows=Query World $sql;$rows | Set-Content "$out/replay.tsv"
$a=[Array]::IndexOf($rows,'FIRST');$b=[Array]::IndexOf($rows,'SECOND');$e=[Array]::IndexOf($rows,'END')
if($a -lt 0 -or $b -le $a -or $e -le $b){throw 'Missing replay markers'}
if(($rows[($a+1)..($b-1)] -join "`n") -ne ($rows[($b+1)..($e-1)] -join "`n")){throw 'Threat migration is not idempotent'}
foreach($expected in @("preserved-sai`t0","no-timer-credit`t0","condition`t23`t7541`t168`t0`t9`t0`t44771`t0","condition`t23`t7541`t168`t1`t9`t0`t42004`t0")){
 if($rows -notcontains $expected){throw "Threat replay mismatch: $expected"}
}
foreach($spell in @(229547,229539)){
 foreach($suffix in @("1`t0`t229539`t1","8`t0`t44720`t0","9`t0`t44771`t0")){
  if($rows -notcontains "condition`t17`t0`t$spell`t0`t$suffix"){throw 'Acquisition gate missing'}
 }
}
$live=Query World @'
SELECT 'keg',entry,type,displayId,flags,Data0,Data10,Data22,Data23,ScriptName FROM gameobject_template WHERE entry=266054;
SELECT 'keg-spawn',guid,id,map,zoneId,areaId,PhaseId,position_x,position_y,position_z FROM gameobject WHERE id=266054;
SELECT 'objective',QuestID,Type,ObjectID,Amount,StorageIndex FROM quest_objectives WHERE QuestID=44771;
SELECT 'chain',ID,PrevQuestID,NextQuestID FROM quest_template_addon WHERE ID IN(44771,44721);
SELECT 'conversation',entry,id FROM conversation_data WHERE entry=3931;
SELECT 'havi-ai',AIName,ScriptName FROM creature_template WHERE entry=92539;
'@
$live | Set-Content "$out/world.tsv"
foreach($expected in @("keg`t266054`t10`t15458`t0`t2617`t229539`t45843`t1`t","keg-spawn`t25210031`t266054`t1220`t7541`t7803`t7510`t3186.02`t1521.32`t181.769","objective`t44771`t0`t114962`t1`t0","chain`t44771`t44720`t44721","chain`t44721`t44771`t44729","conversation`t3931`t8966","havi-ai`tSmartAI`t")){
 if($live -notcontains $expected.TrimEnd([char[]]"`r`n")){throw "Original content contract changed: $expected"}
}
$hotfix=Query Hotfix "SELECT ID,Text FROM broadcast_text WHERE ID=123956; SELECT 'overlay',COUNT(*) FROM player_condition WHERE ID=45843; SELECT 'lock-overlay',COUNT(*) FROM ``lock`` WHERE ID=2617;"
$hotfix | Set-Content "$out/hotfix.tsv"
if($hotfix -notcontains "123956`tOh, the grog. Yes, thank you." -or $hotfix -notcontains "overlay`t0" -or $hotfix -notcontains "lock-overlay`t0"){throw 'Native catalog overlay or delivery text changed'}
function Extract([string]$source,[string]$first,[string]$last,[string]$file){$a=$source.IndexOf($first);$b=$source.IndexOf($last,$a);if($a -lt 0 -or $b -le $a){throw "Production boundary missing: $file"};[IO.File]::WriteAllText("$out/$file.inc",$source.Substring($a,$b-$a))}
$source=Get-Content "$repo/src/server/scripts/Legion/zone_stormheim.cpp" -Raw
Extract $source 'namespace StormheimThreat' 'void AddSC_stormheim()' 'ThreatProduction'
if(!$source.Contains('new player_stormheim_threat();')){throw 'Missing PlayerScript registration'}
$source=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
Extract $source '        case CONDITION_AURA:' '        case CONDITION_ITEM:' 'ThreatAuraCondition'
Extract $source '        case CONDITION_QUESTREWARDED:' '        case CONDITION_QUESTTAKEN:' 'ThreatRewardCondition'
Extract $source '        case CONDITION_QUESTTAKEN:' '        case CONDITION_QUEST_COMPLETE:' 'ThreatActiveCondition'
Extract $source 'bool PlayerConditionLogic(' 'bool ConditionMgr::IsPlayerMeetingCondition(' 'ThreatLogic'
Extract $source 'bool ConditionMgr::IsObjectMeetToConditionList(' 'bool ConditionMgr::IsObjectMeetToConditions(WorldObject*' 'ThreatConditionList'
#Regenerate native quest-credit and chain includes; do not rely on cached handlers.
& "$PSScriptRoot/Test-StormheimCall.ps1" *> "$out/upstream.log"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-call/*.inc" | Copy-Item -Destination $out
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$release.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')
$libs=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($name in @('StormheimThreatCatalog','StormheimThreatTest','StormheimThreatCreditTest')){
 $arguments=@('/nologo','/EHsc','/std:c++20','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/DBOOST_ALL_NO_LIB')
 $arguments+=($includes | ForEach-Object {'/I"'+$_+'"'});$arguments+='/I"'+$out+'"';$arguments+='"'+$PSScriptRoot+'/'+$name+'.cpp"';$arguments+='/Fe"'+$out+'/'+$name+'.exe"';$arguments+='/Fo"'+$out+'/'+$name+'.obj"'
 if($name -like '*Catalog'){$arguments+='/link';$arguments+=($libs | ForEach-Object {'"'+$_+'"'})}
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($arguments -join ' ')+"`nexit /b %errorlevel%`n");& "$out/build.cmd"
 if($LASTEXITCODE){throw "Threat compilation failed: $name"}
}
$previous=$env:PATH
try{$env:PATH="$repo/build-extractors/bin/Release;$env:PATH";foreach($name in @('StormheimThreatCatalog','StormheimThreatTest','StormheimThreatCreditTest')){
 if($name -like '*Catalog'){$result=@(& "$out/$name.exe" "$repo/build-extractors/bin/Release/dbc/enUS/")}else{$result=@(& "$out/$name.exe")};$exit=$LASTEXITCODE;$result | Set-Content "$out/$name.log";$result
 if($exit){throw "Threat native check failed: $name/$exit"}
}}finally{$env:PATH=$previous}
if($VerifyReleaseBinary){
 $exe="$repo/build-extractors/bin/Release/worldserver.exe"
 $bytes=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe));if(!$bytes.Contains('player_stormheim_threat')){throw 'Canonical Release lacks the new registration'}
 $map=Get-Content "$repo/build-extractors/bin/Release/worldserver.map" -Raw
 foreach($symbol in @('?Update@StormheimThreat@@','?OnUpdate@player_stormheim_threat@@','?KilledMonsterCredit@Player@@')){if(!$map.Contains($symbol)){throw "Canonical map lacks $symbol"}}
 foreach($name in @('scripts','game')){
  $relative=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -match "$name\\Release\\$name.lib$"} | Select-Object -First 1
  if(!$relative){throw "Missing actual $name final-link input"}
  $lib=[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $relative))
  if(!(Test-Path $lib) -or (Get-Item $lib).LastWriteTimeUtc -gt (Get-Item $exe).LastWriteTimeUtc){throw "Release predates actual $name library"}
 }
 function Symbol([string]$name){
  $index=$map.IndexOf($name);if($index -lt 0){throw "Missing canonical symbol: $name"}
  $line=$map.Substring($index,$map.IndexOf("`n",$index)-$index)
  $m=[regex]::Match($line,'\s([0-9A-Fa-f]{16})\s+f')
  if(!$m.Success){throw "Missing canonical symbol address: $name"};[Convert]::ToInt64($m.Groups[1].Value,16)
 }
 $binary=[IO.File]::ReadAllBytes($exe);$pe=[BitConverter]::ToInt32($binary,0x3c);$optional=$pe+24
 $imageBase=[BitConverter]::ToInt64($binary,$optional+24);$sections=$optional+[BitConverter]::ToUInt16($binary,$pe+20)
 function Offset([long]$address){
  $relative=$address-$imageBase
  for($i=0;$i -lt [BitConverter]::ToUInt16($binary,$pe+6);$i++){
   $section=$sections+40*$i;$start=[BitConverter]::ToUInt32($binary,$section+12)
   $size=[Math]::Max([BitConverter]::ToUInt32($binary,$section+8),[BitConverter]::ToUInt32($binary,$section+16))
   if($relative -ge $start -and $relative -lt $start+$size){return [long]([BitConverter]::ToUInt32($binary,$section+20)+$relative-$start)}
  };throw 'Address outside canonical PE'
 }
 function Resolve([long]$address){
  for($i=0;$i -lt 10;$i++){$at=Offset $address;if($binary[$at] -ne 0xe9){return $address};$address+=5+[BitConverter]::ToInt32($binary,$at+1)}
  throw 'Unexpected incremental-link thunk cycle'
 }
 function Calls([string]$name){
  $address=Symbol $name;$index=$map.IndexOf($name);$after=$map.IndexOf("`n",$index)+1
  $tail=$map.Substring($after,[Math]::Min(8192,$map.Length-$after));$end=$address+4096
  foreach($m in [regex]::Matches($tail,'\s([0-9A-Fa-f]{16})\s+f')){$v=[Convert]::ToInt64($m.Groups[1].Value,16);if($v -gt $address){$end=[Math]::Min($end,$v)}}
  $at=Offset $address
  for($i=0;$i -lt $end-$address-4;$i++){
   if($binary[$at+$i] -in @(0xe8,0xe9)){try{Resolve ($address+$i+5+[BitConverter]::ToInt32($binary,$at+$i+1))}catch{}}
  }
 }
 $delivery=Resolve (Symbol '?Update@StormheimThreat@@');$credit=Resolve (Symbol '?KilledMonsterCredit@Player@@')
 $actualCalls=@(Calls '?Update@StormheimThreat@@')
 if($actualCalls -notcontains $credit){throw 'Canonical delivery machine code does not call native personal credit'}
 foreach($callback in @('OnUpdate','OnLogin','OnMapChanged')){
  if(@(Calls "?$callback@player_stormheim_threat@@") -notcontains $delivery){throw "Canonical $callback does not reach delivery/lifecycle code"}
 }
 'PASS: canonical Release actual final-link inputs and PE calls: all three callbacks reach delivery; delivery calls native personal credit (incremental thunks resolved).'
}
'PASS: repeated temporary-table migration, native acquisition/delivery metadata and executable production callbacks. WoW client verification remains pending.'
