param([switch]$VerifyReleaseBinary,[string]$ReleaseDirectory="$PSScriptRoot/../../build-extractors/bin/Release")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/hunter-pet-auras"
New-Item -ItemType Directory -Force $out | Out-Null
$source=Get-Content "$repo/src/server/game/Entities/Pet/Pet.cpp" -Raw
$a=$source.IndexOf('void Pet::CastPetAuras(');$b=$source.IndexOf('bool Pet::IsPetAura(',$a)
if($a -lt 0 -or $b -lt $a){throw 'Missing production pet aura dispatcher'}
[IO.File]::WriteAllText("$out/CastPetAuras.inc",$source.Substring($a,$b-$a))
$load=[regex]::Match($source,'(?m)^\s*CastPetAuras\(getPetType\(\) == HUNTER_PET \? IsAlive\(\) : current\);')
$create=[regex]::Match($source,'(?m)^\s*CastPetAuras\(getPetType\(\) == HUNTER_PET && IsAlive\(\)\);')
if(!$load.Success -or !$create.Success){throw 'Missing hunter pet load/create aura lifecycle calls'}
[IO.File]::WriteAllText("$out/Loaded.inc",$load.Value)
[IO.File]::WriteAllText("$out/Created.inc",$create.Value)
$source=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw
$a=$source.IndexOf('        if (m_spellInfo->AuraRestrictions.CasterAuraSpell &&');$b=$source.IndexOf('        if (m_spellInfo->AuraRestrictions.ExcludeCasterAuraSpell',$a)
if($a -lt 0 -or $b -lt $a){throw 'Missing native required-aura guard'}
[IO.File]::WriteAllText("$out/RequiredAura.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/HunterPetAurasTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif %errorlevel% neq 0 exit /b %errorlevel%`n`"$out/test.exe`"`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native hunter pet aura regression failed'}
$sql="SELECT COUNT(*) FROM spell_pet_auras WHERE petEntry=-1 AND spellId=166615 AND `option`=0 AND target=1 AND targetaura=1 AND aura=137015 AND casteraura=0 AND createdspell=0 AND fromspell=0;"
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$old=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$c[3];$rows=@($sql | & C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Hunter pet aura data check failed'}}finally{$env:MYSQL_PWD=$old}
if($rows -notcontains '1'){throw 'Native Beast Mastery Pet Active aura binding is missing or changed'}
# Read the actual runtime DB2 with the production reader, not fixed-field offsets.
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@("$repo/src/common/collision/Management","$repo/src/common/collision","$repo/dep/g3dlite/include",'C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')
$libs=$group.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$compilerArgs=@('/nologo','/EHsc','/std:c++17','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
$compilerArgs+=($includes | ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='"'+$PSScriptRoot+'/HunterPetAuraCatalog.cpp"';$compilerArgs+='/Fe"'+$out+'/catalog.exe"';$compilerArgs+='/Fo"'+$out+'/catalog.obj"';$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})
[IO.File]::WriteAllText("$out/catalog.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n")
& "$out/catalog.cmd"
if($LASTEXITCODE){throw 'Native hunter pet aura catalog build failed'}
$oldPath=$env:PATH
try{$env:PATH="$repo/build-extractors/bin/Release;$env:PATH";& "$out/catalog.exe" "$repo/build-extractors/bin/Release/dbc/enUS/";if($LASTEXITCODE){throw 'Native hunter pet aura DB2 check failed'}}finally{$env:PATH=$oldPath}
if($VerifyReleaseBinary){
 $python=(Get-Command python -ErrorAction SilentlyContinue | Select-Object -First 1).Source
 if(!$python){$python='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'}
 & $python "$PSScriptRoot/verify_hunter_pet_auras_release.py" $ReleaseDirectory
 if($LASTEXITCODE){throw 'Deployed hunter pet aura implementation check failed'}
}
'PASS: production pet aura dispatcher and spell requirement, persistent taming origin, apply/remove/reapply, filters and unrelated pets.'
