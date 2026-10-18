$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/lost-army-landing"
New-Item -ItemType Directory -Force $out | Out-Null
function Query([string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Landing SQL regression failed'}}finally{$env:MYSQL_PWD=$previous};$rows
}
$migration=Get-Content "$repo/sql/updates/world/2026_10_18_04_lost_army_player_progress.sql" -Raw
$scope='SourceTypeOrReferenceId=1 AND SourceEntry=142487 AND SourceGroup IN(115025,115031,115751,115846,116180,116600,116601)'
$select="SELECT SourceGroup,ConditionTypeOrReference,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition FROM conditions WHERE $scope ORDER BY SourceGroup,ConditionTypeOrReference;"
$owned='((SourceGroup IN(115025,115031,115751) AND ConditionTypeOrReference=9 AND ConditionValue1=44737) OR (SourceGroup IN(115846,116180,116600,116601) AND ConditionValue1=44923 AND (ConditionTypeOrReference=9 OR (ConditionTypeOrReference=41 AND ConditionValue2=115845))) OR (ConditionTypeOrReference=58 AND ConditionValue1=0 AND ConditionValue2=1 AND ConditionValue3=0))'
$unrelated="NOT ($scope AND SourceId=0 AND ElseGroup=0 AND $owned)"
$checksum="SELECT COUNT(*),COALESCE(SUM(CRC32(CONCAT_WS('|',SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment))),0) FROM conditions WHERE $unrelated;"
$rows=@(Query @"
CREATE TEMPORARY TABLE saved_conditions LIKE conditions;
INSERT INTO saved_conditions SELECT * FROM conditions;
CREATE TEMPORARY TABLE conditions LIKE saved_conditions;
INSERT INTO conditions SELECT * FROM saved_conditions;
SELECT 'UNRELATED-BEFORE'; $checksum
$migration
SELECT 'FIRST'; $select
$migration
SELECT 'SECOND'; $select
SELECT 'UNRELATED-AFTER'; $checksum
SELECT 'END';
"@)
$a=[Array]::IndexOf($rows,'FIRST');$b=[Array]::IndexOf($rows,'SECOND');$c=[Array]::IndexOf($rows,'UNRELATED-AFTER')
if($a -lt 0 -or $b-$a -ne 26 -or $c-$b -ne 26 -or ($rows[($a+1)..($b-1)] -join "`n") -ne ($rows[($b+1)..($c-1)] -join "`n")){throw 'Landing migration is not idempotent or scoped to25 conditions'}
if($rows[1] -ne $rows[$c+1]){throw 'Unrelated conditions changed'}
$conditions=$rows[($a+1)..($b-1)]
foreach($entry in @(115025,115031,115751,115846,116180,116600,116601)){
 $wq=if($entry -in @(115025,115031,115751)){44737}else{44923}
 foreach($type in @(9,60)){if($conditions -notcontains "$entry`t$type`t$wq`t0`t0`t0"){throw "Missing condition$type on$entry"}}
 if($conditions -notcontains "$entry`t58`t0`t1`t0`t0"){throw "Missing raid exclusion on$entry"}
 if($wq -eq 44923 -and $conditions -notcontains "$entry`t41`t44923`t115845`t10`t1"){throw "Missing Vrekt quota condition on$entry"}
}
$conditions | Set-Content "$out/conditions.tsv"
Query 'SELECT Entry,Item,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount FROM creature_loot_template WHERE Item=142487 ORDER BY Entry;' | Set-Content "$out/loot.tsv"
$objectives=@(Query 'SELECT QuestID,ObjectID,Amount,StorageIndex FROM quest_objectives WHERE QuestID=44923 ORDER BY StorageIndex;')
if(($objectives -join ',') -ne "44923`t115845`t10`t0,44923`t115846`t1`t1"){throw 'Vrekt native objective mapping changed'}
$objectives | Set-Content "$out/objectives.tsv"

& "$PSScriptRoot/Test-LostArmy.ps1" *> "$out/base.log"
Get-ChildItem "$repo/build-extractors/runtime-test/lost-army/*.inc" | Copy-Item -Destination $out
$base=Get-Content "$out/LostArmyBaseFixture.inc" -Raw
$base=$base.Replace('struct Unit:WorldObject {','struct Unit:WorldObject {struct Threat{Unit* getHostilTarget(){return nullptr;}};Threat& getThreatManager(){static Threat t;return t;}bool isPet(){return false;}Unit* GetOwner(){return nullptr;}')
$base=$base.Replace('bool isRaidGroup(){return false;}','bool raid=false;bool isRaidGroup(){return raid;}')
$old='int32 GetQuestObjectiveData(uint32 q,uint32 e)const{return data[{q,e}];}'
if(!$base.Contains($old)){throw 'Missing fixture objective lookup boundary'}
$base=$base.Replace($old,'int32 GetQuestObjectiveData(uint32,uint32)const;')
$base=$base.Replace('return GetQuestObjectiveData(q->id,o.ObjectID);return 0;}','return data[{q->id,o.ObjectID}];return 0;}')
[IO.File]::WriteAllText("$out/LostArmyBaseFixture.inc",$base)
$fixture=Get-Content "$out/LostArmyFixture.inc" -Raw
$fixture=$fixture.Replace('IsObjectMeetToConditions(Player*,','IsObjectMeetToConditions(Player* player,').Replace('c->Meets()','c->Meets(player)')
[IO.File]::WriteAllText("$out/LostArmyFixture.inc",$fixture)
function Export([string]$source,[string]$start,[string]$end,[string]$file){
 $a=$source.IndexOf($start);$b=$source.IndexOf($end,$a)
 if($a -lt 0 -or $b -le $a){throw "Missing production boundary$file"}
 [IO.File]::WriteAllText("$out/$file.inc",$source.Substring($a,$b-$a))
}
$native=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
Export $native '        case CONDITION_QUESTTAKEN:' '        case CONDITION_QUEST_COMPLETE:' 'LandingQuestTaken'
Export $native '        case CONDITION_QUEST_OBJECTIVE_DONE:' '        case CONDITION_INSTANCE_INFO:' 'LandingObjectiveDone'
Export $native '        case CONDITION_IN_RAID_OR_GROUP:' '        case CONDITION_WORLD_QUEST:' 'LandingGroup'
Export $native '    if (NegativeCondition)' '    if (!condMeets)' 'LandingNegation'
$native=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
Export $native 'int32 Player::GetQuestObjectiveData(uint32' 'int32 Player::GetQuestObjectiveData(Quest const*' 'LandingObjectiveLookup'
$oldTest=Get-Content "$PSScriptRoot/LostArmyLootTest.cpp" -Raw
$gate=$oldTest.Substring($oldTest.IndexOf('namespace Gate {'),$oldTest.IndexOf('constexpr int CONDITION_WORLD_QUEST')-$oldTest.IndexOf('namespace Gate {'))
[IO.File]::WriteAllText("$out/LandingWorldQuestFixture.inc",$gate)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/LostArmyLandingTest.cpp`" /Fe`"$out/Landing.exe`" /Fo`"$out/Landing.obj`"`nif not %errorlevel%==0 exit /b %errorlevel%`n`"$out/Landing.exe`" `"$out/conditions.tsv`"`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd)
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native landing loot regression failed'}
'PASS: repeated migration, actual native condition9/41/58/60 and item-objective lookup, per-player quota/completion/raid gating. Client and full live loot dispatch remain pending.'
