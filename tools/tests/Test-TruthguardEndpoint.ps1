param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Test-TruthguardRider.ps1" -EndpointTest -EndpointBaseline:$Baseline -VerifyReleaseBinary:$VerifyReleaseBinary
if($LASTEXITCODE){throw 'Truthguard endpoint host failed'}
if($VerifyReleaseBinary){
 $repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
 $out="$repo/build-extractors/runtime-test/truthguard-rider"
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$live=@("SET SESSION TRANSACTION READ ONLY; SELECT id,target_map,target_position_x,target_position_y,target_position_z,target_orientation FROM spell_target_position WHERE id IN(209908,210360) ORDER BY id;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Endpoint destination read failed'}}finally{$env:MYSQL_PWD=$previous}
 [IO.File]::WriteAllLines("$out/endpoint-live.tsv",[string[]]$live)
 if(Compare-Object @("209908`t1495`t4761.75`t67.94`t-2.1`t1.07","210360`t1220`t-837.16`t4259.31`t746.26`t1.99") $live){throw 'Native endpoint destinations changed'}
 $native=Get-Content "$out/native.tsv"
 foreach($expected in @("EFFECT`t209908`t0`t90`t0`t105889`t0`t1`t0","EFFECT`t209908`t1`t227`t0`t1333`t0`t1`t17","EFFECT`t210360`t0`t90`t0`t105892`t0`t1`t0","EFFECT`t210360`t1`t252`t0`t1317`t0`t1`t17","CAST`t209908`t1`t0`t0","CAST`t210360`t1`t0`t0")){if($native -notcontains $expected){throw "Native endpoint effect/cast changed: $expected"}}
}
