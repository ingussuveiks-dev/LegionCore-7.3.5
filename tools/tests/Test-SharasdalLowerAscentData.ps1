param([string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-lower-ascent-data"};$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
$prior=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile SharasdalAscentCatalog.cpp -ProbeName SharasdalLowerAscentCatalog -OutputFile "$out/native.tsv";if(!$?){throw 'Matching native ascent metadata failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
$native=@(Get-Content "$out/native.tsv");foreach($line in @("EFFECT`t208580`t0`t0`t6`t430`t1247`t0`t25`t0","EFFECT`t209992`t0`t0`t252`t0`t0`t0`t1`t17","EFFECT`t209992`t0`t1`t164`t0`t0`t208580`t1`t0","EFFECT`t209992`t0`t2`t61`t0`t50336`t0`t1`t0","TREE`t2`t47127`t47126`t0`t1`t29660","TREE`t2`t47330`t47126`t0`t1`t29774","TREE`t3`t47131`t47130`t0`t1`t29795","TREE`t3`t47378`t47130`t0`t1`t29661","EOF`tascent-native")){if($native -notcontains $line){throw "Native ascent changed: $line"}}
if(@($native|Where-Object {$_ -like "EFFECT`t208580`t*"}).Count -ne 1 -or @($native|Where-Object {$_ -like "EFFECT`t209992`t*"}).Count -ne 3){throw 'Native camera/exit effect scope changed'}
$sql=@'
SET SESSION TRANSACTION READ ONLY;
SELECT 'path',point,position_x,position_y,position_z,orientation,move_type,speed,delay,action,action_chance FROM waypoint_data_script WHERE id=10993810 ORDER BY point;
SELECT 'actor',guid,id,map,PhaseID FROM creature WHERE guid IN(362063,362066,362078,362093) ORDER BY guid;
SELECT 'objective',ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE ID=283383;
SELECT 'profile',SourceTypeOrReferenceId,SourceGroup,SourceEntry,COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId=17 AND SourceEntry IN(208580,209992)) OR(SourceTypeOrReferenceId=18 AND SourceGroup=38821 AND SourceEntry=208580) GROUP BY SourceTypeOrReferenceId,SourceGroup,SourceEntry ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry;
SELECT 'EOF','lower-ascent';
'@
[IO.File]::WriteAllText("$out/read.sql",$sql)
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';');$prior=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$code=& "$PSScriptRoot/Invoke-NativeSqlFiles.ps1" -Executable C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe -Arguments @("--host=$($connection[0])","--port=$($connection[1])","--user=$($connection[2])","--database=$($connection[4])",'--batch','--raw','--skip-column-names') -InputFile "$out/read.sql" -OutputFile "$out/world.tsv" -ErrorFile "$out/world.stderr";if($code -or (Get-Item "$out/world.stderr").Length){throw 'Complete native lower-ascent World read failed'}}finally{$env:MYSQL_PWD=$prior}
$world=@(Get-Content "$out/world.tsv");foreach($line in @("actor`t362063`t102826`t1600`t6120","actor`t362066`t105298`t1600`t6120","actor`t362078`t104853`t1600`t6120","actor`t362093`t102826`t1600`t6120","objective`t283383`t40341`t0`t0`t105805`t1`t0","profile`t17`t0`t208580`t12","profile`t17`t0`t209992`t12","profile`t18`t38821`t208580`t12","EOF`tlower-ascent")){if($world -notcontains $line){throw "Lower-ascent identity/admission changed: $line"}}
$paths=@($world|Where-Object {$_ -like "path`t*"});if($paths.Count -ne 6 -or $world.Count -ne 15){throw 'Lower-ascent path/context count changed'}
$expected=@(@(1,-353.54,807.974,258.951),@(2,-315.37,809.207,261.249),@(3,-295.28,809.113,260.844),@(4,-279.015,809.036,259.377),@(6,-203.135,808.68,261.464),@(7,-205.608,808.472,261.755))
$export=@();for($i=0;$i -lt 6;++$i){$f=$paths[$i].Split("`t");if($f.Count -ne 11 -or [int]$f[1] -ne $expected[$i][0] -or [int]$f[6] -lt 0 -or [int]$f[6] -gt 3 -or [int]$f[9] -ne 0){throw 'Native path movement/event changed'};for($j=1;$j -le 3;++$j){if([Math]::Abs([double]::Parse($f[$j+1],[Globalization.CultureInfo]::InvariantCulture)-$expected[$i][$j]) -gt 0.0001){throw 'Native path pose changed'}};$export+=($f[1..10] -join "`t")}
[IO.File]::WriteAllLines("$out/path.tsv",[string[]]$export)
'PASS fresh26972/read-onlyHotfix native camera/exit/real root2+3 and fullEOF World: exact lower roles/persisted105805 witness/installed six-node10993810 with native final index5, no node event producer, retained World05 three12-row profiles. No mutation or actual client traversal claim.'
