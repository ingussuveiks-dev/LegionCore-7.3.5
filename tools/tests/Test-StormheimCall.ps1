$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-call"
New-Item -ItemType Directory -Force $out | Out-Null
function Read-Call([string]$database,[string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match "^${database}DatabaseInfo\s*="} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Call to Action data query failed'}}finally{$env:MYSQL_PWD=$previous}
 @($rows | ForEach-Object {$_.ToString().TrimEnd([char[]]"`r`n")})
}
$sql=@'
SELECT 'quest',q.ID,q.Flags,q.FlagsEx,q.AllowableRaces,q.StartItem,a.ProvidedItemCount,q.RewardNextQuest,a.PrevQuestID,a.NextQuestID,a.SpecialFlags,q.MinLevel,a.MaxLevel,q.QuestType,q.QuestInfoID FROM quest_template q JOIN quest_template_addon a USING(ID) WHERE q.ID IN(40072,44720,44771);
SELECT 'objective',QuestID,ID,Type,ObjectID,Amount,StorageIndex,Flags FROM quest_objectives WHERE QuestID IN(44720,44771);
SELECT 'starter',id,quest FROM creature_queststarter WHERE quest IN(44720,44771);
SELECT 'ender',id,quest FROM creature_questender WHERE quest IN(44720,44771);
SELECT '44720-relations',(SELECT COUNT(*) FROM creature_queststarter WHERE quest=44720),(SELECT COUNT(*) FROM creature_questender WHERE quest=44720);
SELECT 'actor',w.Entry,w.Displayid1,w.Displayid2,w.Displayid3,w.Displayid4,t.faction,t.npcflag,t.unit_flags FROM creature_template_wdb w JOIN creature_template t ON t.entry=w.Entry WHERE w.Entry IN(114946,92539);
SELECT 'spawn',guid,id,map,zoneId,areaId,phaseMask,PhaseId,spawnMask,modelid,position_x,position_y,position_z FROM creature WHERE id IN(114946,92539);
SELECT 'extra-gates',AllowableClasses,SourceSpellID,ExclusiveGroup,RequiredSkillID,RequiredSkillPoints,RequiredMinRepFaction,RequiredMaxRepFaction FROM quest_template_addon WHERE ID=44720;
SELECT 'conditions',COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId IN(19,20) AND SourceEntry=44720;
SELECT 'disables',COUNT(*) FROM disables WHERE sourceType=1 AND entry=44720;
SELECT 'go-relations',COUNT(*) FROM(SELECT quest FROM gameobject_queststarter UNION ALL SELECT quest FROM gameobject_questender)r WHERE quest=44720;
SELECT 'model-metadata',COUNT(*) FROM creature_model_info WHERE DisplayID IN(59729,76630);
'@
$rows=Read-Call World $sql;$rows | Set-Content "$out/world.tsv"
foreach($row in @("quest`t44720`t34603016`t0`t18446744073709551615`t0`t0`t44771`t40072`t44771`t0`t110`t0`t2`t0","starter`t114946`t44720","starter`t92539`t44771","ender`t92539`t44720","ender`t92539`t44771","44720-relations`t1`t1","actor`t92539`t76630`t0`t0`t0`t35`t3`t32768","actor`t114946`t59729`t0`t0`t0`t35`t2`t0","extra-gates`t0`t0`t0`t0`t0`t0`t0","conditions`t0","disables`t0","go-relations`t0","model-metadata`t2")){
 if($rows -notcontains $row){throw "Quest contract changed: $row"}
}
if(@($rows | Where-Object {$_ -like "objective`t44720`t*"}).Count){throw 'Travel quest gained an objective'}
$spawns=@($rows | Where-Object {$_ -like "spawn`t*"});if($spawns.Count -ne 2){throw 'Expected original Muninn and Havi spawns'}
foreach($row in $spawns){$v=$row.Split("`t");if($v[3] -ne '1220' -or $v[6] -ne '1' -or $v[7] -ne '' -or $v[8] -ne '1' -or $v[9] -ne '0'){throw "Spawn phase/model override changed: $row"}}
foreach($prefix in @("spawn`t291280`t92539`t1220`t7541`t7803`t","spawn`t14208136`t114946`t1220`t7502`t7505`t")){if(!@($spawns | Where-Object {$_.StartsWith($prefix)}).Count){throw 'Original NPC identity/area changed'}}
foreach($row in @($rows | Where-Object {$_ -like "actor`t*"})){$v=$row.Split("`t");if(([int]$v[7] -band 2) -eq 0){throw 'NPC lacks questgiver flag'}}
$hotfix=Read-Call Hotfix "SELECT 'line',COUNT(*) FROM quest_line_x_quest WHERE QuestID IN(44720,44771); SELECT 'display',COUNT(*) FROM creature_display_info WHERE ID IN(59729,76630); SELECT 'model',COUNT(*) FROM creature_model_data; SELECT 'area',COUNT(*) FROM area_table WHERE ID IN(7505,7803);"
$hotfix | Set-Content "$out/hotfix.tsv";foreach($kind in @('line','display','model','area')){if($hotfix -notcontains "$kind`t0"){throw "Unexpected $kind Hotfix overlay; inspect before accepting native-only evidence"}}

# Reuse the existing native quest/session fixture, then replace its level/chain adapters.
& "$PSScriptRoot/Test-StormheimJournals.ps1" *> "$out/upstream.log"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-journals/*.inc" | Copy-Item -Destination $out
$fixture=Get-Content "$out/HelheimSuppliesFixture.inc" -Raw
$fixture=$fixture.Replace('struct Quest {','using PrevChainQuests=std::vector<uint32>;struct Quest {uint32 MinLevel=0,MaxLevel=0;PrevChainQuests prevChainQuests;')
$fixture=$fixture.Replace('struct Player:Unit {','struct Player:Unit {uint32 level=110;uint32 getLevel(){return level;}')
$old='void AddQuestAndCheckCompletion(Quest const* q,WorldObject*){Accept(q->id);}'
if(!$fixture.Contains($old)){throw 'Acceptance fixture boundary changed'}
$fixture=$fixture.Replace($old,'void AddQuest(Quest const* q,WorldObject*){Accept(q->id);}void AddQuestAndCheckCompletion(Quest const*,WorldObject*);')
foreach($name in @('Level','NextChain','PrevChain')){
 $old="bool SatisfyQuest$name(Quest const*,bool){return true;}"
 if(!$fixture.Contains($old)){throw "Native fixture adapter changed: $name"}
 $fixture=$fixture.Replace($old,"bool SatisfyQuest$name(Quest const*,bool);")
}
[IO.File]::WriteAllText("$out/HelheimSuppliesFixture.inc",$fixture)
$prefix=Get-Content "$PSScriptRoot/StormheimJournalsTest.cpp" -Raw
[IO.File]::WriteAllText("$out/CallFixture.inc",$prefix.Substring(0,$prefix.IndexOf('int main(')))
$core=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$core.IndexOf('void Player::AddQuestAndCheckCompletion(');$a=$core.IndexOf('{',$a)+1;$b=$core.IndexOf('    if (!questGiver)',$a)
if($a -le 0 -or $b -le $a){throw 'Native acceptance completion block missing'}
[IO.File]::WriteAllText("$out/CallAccept.inc",$core.Substring($a,$b-$a))
foreach($part in @(@('CallLevel','bool Player::SatisfyQuestLevel(','bool Player::SatisfyQuestLog('),@('CallNextChain','bool Player::SatisfyQuestNextChain(','bool Player::SatisfyQuestPrevChain('),@('CallPrevChain','bool Player::SatisfyQuestPrevChain(','bool Player::SatisfyQuestDay('))){
 $a=$core.IndexOf($part[1]);$b=$core.IndexOf($part[2],$a);if($a -lt 0 -or $b -le $a){throw 'Native gate missing'}
 [IO.File]::WriteAllText("$out/$($part[0]).inc",$core.Substring($a,$b-$a))
}
$loader=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
$a=$loader.IndexOf('        if (qinfo->NextQuestIdChain)');$b=$loader.IndexOf('        for (uint8 j = 0; j < QUEST_REWARD_CURRENCY_COUNT;',$a)
if($a -lt 0 -or $b -le $a){throw 'Native chain loader missing'}
[IO.File]::WriteAllText("$out/CallChainLoader.inc",$loader.Substring($a,$b-$a))
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$release=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$release.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')
$libs=$release.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($name in @('StormheimCallCatalog','StormheimCallTest')){
 $arguments=@('/nologo','/EHsc','/std:c++20','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/DBOOST_ALL_NO_LIB')
 $arguments+=($includes | ForEach-Object {'/I"'+$_+'"'});$arguments+='/I"'+$out+'"';$arguments+='"'+$PSScriptRoot+'/'+$name+'.cpp"';$arguments+='/Fe"'+$out+'/'+$name+'.exe"';$arguments+='/Fo"'+$out+'/'+$name+'.obj"'
 if($name -like '*Catalog'){$arguments+='/link';$arguments+=($libs | ForEach-Object {'"'+$_+'"'})}
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($arguments -join ' ')+"`nexit /b %errorlevel%`n");& "$out/build.cmd"
 if($LASTEXITCODE){throw "Compilation failed: $name"}
}
$previous=$env:PATH
try{
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 $native=@(& "$out/StormheimCallCatalog.exe" "$repo/build-extractors/bin/Release/dbc/enUS/");$code=$LASTEXITCODE;$native | Set-Content "$out/native.txt";$native
 if($code){throw "Native catalog failed: $code"}
 & "$out/StormheimCallTest.exe" "$out/world.tsv"
 if($LASTEXITCODE){throw 'Native Call to Action quest regression failed'}
}finally{$env:PATH=$previous}
