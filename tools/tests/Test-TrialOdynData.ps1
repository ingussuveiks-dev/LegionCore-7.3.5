$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/trial-odyn-data"
New-Item -ItemType Directory -Force $out | Out-Null
function Query([string]$sql,[string]$database='World'){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match "^${database}DatabaseInfo\s*="} | Select-Object -First 1).Split('"')[1].Split(';');$previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Trial Odyn SQL check failed'}}finally{$env:MYSQL_PWD=$previous};$rows
}
$rows=@(Query @'
SELECT 'objective',COUNT(*) FROM quest_objectives WHERE QuestID=44729 AND ID=286836 AND Type=14 AND ObjectID=55330 AND Amount=1 AND StorageIndex=0 AND Flags=0;
SELECT 'objective-count',COUNT(*) FROM quest_objectives WHERE QuestID=44729;
SELECT 'chain',COUNT(*) FROM quest_template_addon WHERE ID=44729 AND PrevQuestID=44721 AND NextQuestID=44868;
SELECT 'reward',COUNT(*) FROM quest_template WHERE ID=44729 AND RewardItem1=142533 AND RewardAmount1=1 AND RewardNextQuest=44868 AND QuestInfoID=62;
SELECT 'start',COUNT(*) FROM creature_queststarter WHERE quest=44729 AND id=92539;
SELECT 'end',COUNT(*) FROM creature_questender WHERE quest=44729 AND id=114263;
SELECT 'next',COUNT(*) FROM creature_queststarter WHERE quest=44868 AND id=114263;
SELECT 'actor',COUNT(*) FROM creature_template WHERE entry=114263 AND ScriptName='boss_tov_odyn' AND npcflag=3 AND gossip_menu_id=20502;
SELECT 'spawn',COUNT(*) FROM creature WHERE id=114263 AND map=1648 AND guid=505062 AND spawnMask=245760 AND modelid=0;
SELECT 'instance',COUNT(*) FROM instance_template WHERE map=1648 AND script='instance_trial_of_valor';
SELECT 'gossip',COUNT(*) FROM gossip_menu_option WHERE MenuID=20502 AND OptionID=0 AND OptionBroadcastTextID=123259;
'@)
foreach($tag in @('objective','objective-count','chain','reward','start','end','next','actor','spawn','instance','gossip')){if($rows -notcontains "$tag`t1"){throw "Trial Odyn World mismatch: $tag"}}
$rows | Set-Content "$out/world.tsv"
$hotfix=@(Query @'
SELECT 'trees',COUNT(*) FROM criteria_tree WHERE ID IN(55330,55331);
SELECT 'criteria',COUNT(*) FROM criteria WHERE ID=34579;
SELECT 'encounters',COUNT(*) FROM dungeon_encounter WHERE ID IN(1958,1809);
'@ 'Hotfix')
foreach($tag in @('trees','criteria','encounters')){if($hotfix -notcontains "$tag`t0"){throw "Native Trial Odyn data has an overlay to inspect: $tag"}}
$hotfix | Set-Content "$out/hotfix.tsv"
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$r=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$r.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@('src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include') | ForEach-Object {Join-Path $repo $_}
$includes+=@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')
$libs=$r.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='"'+$PSScriptRoot+'/TrialOdynCatalog.cpp"';$compilerArgs+='/Fe"'+$out+'/Catalog.exe"';$compilerArgs+='/Fo"'+$out+'/Catalog.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
[IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n")
& "$out/build.cmd";if($LASTEXITCODE){throw 'Trial Odyn native catalog compilation failed'}
$previousPath=$env:PATH
try{$env:PATH="$repo/build-extractors/bin/Release;$env:PATH";& "$out/Catalog.exe" "$repo/build-extractors/bin/Release/dbc/enUS/" "$out/FavorCatalog.inc";if($LASTEXITCODE){throw 'Trial Odyn native catalog failed'}}finally{$env:PATH=$previousPath}
& "$PSScriptRoot/Test-QuestCriteriaCredit.ps1" *> "$out/quest-criteria-base.log"
Get-ChildItem "$repo/build-extractors/runtime-test/quest-criteria-credit/*.inc" | Copy-Item -Destination $out
$f=Get-Content "$PSScriptRoot/QuestCriteriaCreditTest.cpp" -Raw
[IO.File]::WriteAllText("$out/QuestFixture.inc",$f.Substring(0,$f.IndexOf('int main()')))
$ach=Get-Content "$repo/src/server/game/Achievements/AchievementMgr.cpp" -Raw
$start=$ach.IndexOf('bool AchievementMgr<T>::CanUpdateCriteria')
if($start -lt 0){throw 'Native criteria admission boundary missing'}
$case=$ach.IndexOf('        case CRITERIA_TYPE_COMPLETE_DUNGEON_ENCOUNTER:',$start)
$a=$ach.IndexOf('            if (!miscValue1 || miscValue1 != criteria->Asset)',$case)
$b=$ach.IndexOf('            break;',$a)
if($case -lt $start -or $a -lt $case -or $b -lt $a){throw 'Native encounter asset admission boundary missing'}
[IO.File]::WriteAllText("$out/EncounterAdmission.inc",$ach.Substring($a,$b-$a))
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/TrialOdynCriteriaTest.cpp`" /Fe`"$out/Criteria.exe`" /Fo`"$out/Criteria.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/Criteria.exe`"`r`n"
[IO.File]::WriteAllText("$out/criteria.cmd",$cmd);& "$out/criteria.cmd";if($LASTEXITCODE){throw 'Trial Odyn quest credit regression failed'}
'PASS: live World chain/actor/raid masks/turn-in, no criteria overlays, native DB2 catalog and Player quest credit.'
