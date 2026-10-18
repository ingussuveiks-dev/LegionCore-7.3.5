param([switch]$Live)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/vrekt-poi"
New-Item -ItemType Directory -Force $out | Out-Null
function Query([string]$sql){
 $c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Vrekt POI SQL test failed'}}finally{$env:MYSQL_PWD=$previous}
 $rows
}
$migration=Get-Content "$repo/sql/updates/world/2026_10_18_11_vrekt_corrupt_poi.sql" -Raw
$columns='QuestID,BlobIndex,Idx1,ObjectiveIndex,QuestObjectiveID,QuestObjectID,MapID,WorldMapAreaId,Floor,Priority,Flags,WorldEffectID,PlayerConditionID,WoDUnk1,AlwaysAllowMergingBlobs'
$rows=@(Query @"
CREATE TEMPORARY TABLE saved_poi LIKE quest_poi;
INSERT INTO saved_poi SELECT * FROM quest_poi WHERE QuestID IN(44923,44737);
CREATE TEMPORARY TABLE quest_poi LIKE saved_poi;
INSERT INTO quest_poi SELECT * FROM saved_poi;
CREATE TEMPORARY TABLE saved_points LIKE quest_poi_points;
INSERT INTO saved_points SELECT * FROM quest_poi_points WHERE QuestID IN(44923,44737);
CREATE TEMPORARY TABLE quest_poi_points LIKE saved_points;
INSERT INTO quest_poi_points SELECT * FROM saved_points;
DELETE FROM quest_poi WHERE QuestID=44923 AND BlobIndex=0 AND Idx1=1;
INSERT INTO quest_poi VALUES(44923,0,1,1191182336,-2063596446,-1006632508,-117440508,3,0,0,0,0,0,167772160,0,24015);
SELECT 'before',$columns FROM quest_poi WHERE QuestID=44923 ORDER BY Idx1,BlobIndex;
SELECT 'retained',p.* FROM quest_poi p WHERE NOT(QuestID=44923 AND BlobIndex=0 AND Idx1=1) ORDER BY QuestID,Idx1,BlobIndex;
SELECT 'points-before',p.* FROM quest_poi_points p ORDER BY QuestID,Idx1,Idx2;
$migration
SELECT 'first',p.* FROM quest_poi p ORDER BY QuestID,Idx1,BlobIndex;
SELECT 'after',$columns FROM quest_poi WHERE QuestID=44923 ORDER BY Idx1,BlobIndex;
$migration
SELECT 'repeat',p.* FROM quest_poi p ORDER BY QuestID,Idx1,BlobIndex;
SELECT 'points-after',p.* FROM quest_poi_points p ORDER BY QuestID,Idx1,Idx2;
SELECT 'point-input',QuestID,Idx1,X,Y FROM quest_poi_points WHERE QuestID=44923 ORDER BY Idx1,Idx2;
INSERT INTO quest_poi VALUES(44923,0,1,0,287303,115845,1220,1017,0,0,0,0,0,0,0,0);
$migration
SELECT 'external',COUNT(*) FROM quest_poi WHERE QuestID=44923 AND BlobIndex=0 AND Idx1=1 AND MapID=1220 AND QuestObjectiveID=287303;
"@)
function Tagged([string]$prefix){@($rows | Where-Object {$_ -like "$prefix`t*"} | ForEach-Object {($_ -split "`t",2)[1]})}
$before=Tagged 'before';$after=Tagged 'after'
if($before.Count -ne 5 -or $after.Count -ne 4 -or
 ((Tagged 'retained') -join '|') -ne ((Tagged 'first') -join '|') -or
 ((Tagged 'first') -join '|') -ne ((Tagged 'repeat') -join '|') -or
 ((Tagged 'points-before') -join '|') -ne ((Tagged 'points-after') -join '|') -or "external`t1" -notin $rows){throw 'Scoped POI migration changed valid polygons/pins/neighbour/points or overwrote externally corrected row'}
$before | Set-Content "$out/before.tsv";$after | Set-Content "$out/after.tsv"
$points=Tagged 'point-input'
if($points.Count -ne 21){throw 'Vrekt native polygon/pin point inventory changed'}
$points | Set-Content "$out/points.tsv"
if($Live){
 $liveRows=@(Query "SELECT $columns FROM quest_poi WHERE QuestID=44923 ORDER BY Idx1,BlobIndex;")
 if(($liveRows -join '|') -ne ($after -join '|')){throw 'Live Vrekt POIs differ from tested repair'}
 $livePoints=@(Query 'SELECT QuestID,Idx1,X,Y FROM quest_poi_points WHERE QuestID=44923 ORDER BY Idx1,Idx2;')
 if(($livePoints -join '|') -ne ($points -join '|')){throw 'Live Vrekt point coordinates differ from the preserved fixture'}
}
function Export([string]$source,[string]$start,[string]$end,[string]$name){
 $a=$source.IndexOf($start);$b=$source.IndexOf($end,$a)
 if($a -lt 0 -or $b -le $a){throw "Missing native POI boundary $name"}
 [IO.File]::WriteAllText("$out/$name.inc",$source.Substring($a,$b-$a))
}
$header=Get-Content "$repo/src/server/game/Globals/QuestData.h" -Raw
Export $header 'struct QuestPOIPoint' 'struct PointOfInterest' 'PoiTypes'
$native=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
Export $native 'QuestPOI::QuestPOI()' 'QuestDataStoreMgr::QuestDataStoreMgr()' 'PoiConstructors'
Export $native 'void QuestDataStoreMgr::LoadQuestPOI()' 'void QuestDataStoreMgr::LoadQuestRelationsHelper(' 'PoiLoader'
Export $native 'QuestPOIVector const* QuestDataStoreMgr::GetQuestPOIVector(' 'std::vector<QuestObjective> QuestDataStoreMgr::GetQuestObjectivesByType(' 'PoiLookup'
$handler=Get-Content "$repo/src/server/game/Handlers/QueryHandler.cpp" -Raw
$a=$handler.IndexOf('            WorldPackets::Query::QuestPOIData questPOIData;');$end='            response.QuestPOIDataStats.push_back(questPOIData);';$b=$handler.IndexOf($end,$a)
if($a -lt 0 -or $b -le $a){throw 'Missing native response construction'}
[IO.File]::WriteAllText("$out/PoiResponse.inc",$handler.Substring($a,$b-$a+$end.Length))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /I`"$out`" `"$PSScriptRoot/VrektPoiTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/before.tsv`" `"$out/after.tsv`" `"$out/points.tsv`"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd)
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Vrekt native POI loader/response regression failed'}
'PASS: isolated repeated POI migration preserves all retained rows and21 shared points; actual native loading and response construction execute for both imported-corrupt and repaired data.'
