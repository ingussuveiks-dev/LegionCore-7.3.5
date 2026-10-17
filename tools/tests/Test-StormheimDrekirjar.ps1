$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-StormheimDrekirjarData.ps1"
$out="$repo/build-extractors/runtime-test/stormheim-drekirjar"
& "$PSScriptRoot/Test-StormheimTideskorn.ps1" *> "$out/upstream.log"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-tideskorn/*.inc" | Copy-Item -Destination $out
$f=Get-Content "$out/SpineFixture.inc" -Raw
$old='void SetQuestSlotCounter(uint16,int8 index,int32 n){slotCounters[index]=n;}'
if(!$f.Contains($old)){throw 'Sparse-counter fixture boundary changed'}
$f=$f.Replace($old,'std::map<uint32,uint32> packed;void SetUInt16Value(uint32 field,uint8 half,uint16 count){assert(half<2);auto& word=packed[field];word=(word&~(0xffffu<<(half*16)))|(uint32(count)<<(half*16));}void SetQuestSlotCounter(uint16,uint8,uint16);')
$header=Get-Content "$repo/src/server/game/Entities/Player/Player.h" -Raw
$a=$header.IndexOf('enum QuestSlotOffsets');$b=$header.IndexOf('#define QUESTS_COMPLETED_BITS_SIZE',$a)
if($a -lt 0 -or $b -le $a){throw 'Native quest slot layout missing'}
[IO.File]::WriteAllText("$out/SpineFixture.inc","constexpr unsigned PLAYER_FIELD_QUEST_LOG=0;`n"+$header.Substring($a,$b-$a)+$f)
$native=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$a=$native.IndexOf('void Player::SetQuestSlotCounter(');$b=$native.IndexOf('void Player::SetSpecialCriteriaComplete(',$a)
[IO.File]::WriteAllText("$out/DrekirjarCounter.inc",$native.Substring($a,$b-$a))
$condition=Get-Content "$repo/src/server/game/Conditions/ConditionMgr.cpp" -Raw
$a=$condition.IndexOf('bool ConditionMgr::IsPlayerMeetingCondition(Unit* unit, int32 conditionID');$b=$condition.IndexOf('bool ConditionMgr::IsPlayerMeetingCondition(Unit* unit, PlayerConditionEntry',$a)
[IO.File]::WriteAllText("$out/DrekirjarTaskCondition.inc",$condition.Substring($a,$b-$a))
$a=$condition.IndexOf('        case CONDITION_ALIVE:');$b=$condition.IndexOf('        case CONDITION_HP_VAL:',$a)
[IO.File]::WriteAllText("$out/DrekirjarAlive.inc",$condition.Substring($a,$b-$a))
$go=Get-Content "$repo/src/server/game/Entities/GameObject/GameObject.cpp" -Raw
$a=$go.IndexOf('                // possible quest objective for active quests',$go.IndexOf('void GameObject::Use('));$b=$go.IndexOf('                if (Battleground* bg',$a)
[IO.File]::WriteAllText("$out/DrekirjarGoQuest.inc",$go.Substring($a,$b-$a))
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$r=$project.Project.ItemDefinitionGroup | Where-Object Condition -like '*Release|x64*' | Select-Object -First 1
$includes=$r.ClCompile.AdditionalIncludeDirectories.Split(';') | Where-Object {$_ -and $_ -notlike '%*'}
$includes+=@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include')
$libs=$r.Link.AdditionalDependencies.Split(';') | Where-Object {$_ -and $_ -notlike '%*'} | ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($name in @('StormheimDrekirjarCatalog','StormheimDrekirjarTest')){
 $compilerArgs=@('/nologo','/EHsc','/std:c++20','/MD','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
 $compilerArgs+=($includes | ForEach-Object {'/I"'+$_+'"'});$compilerArgs+='/I"'+$out+'"';$compilerArgs+='"'+$PSScriptRoot+'/'+$name+'.cpp"';$compilerArgs+='/Fe"'+$out+'/'+$name+'.exe"';$compilerArgs+='/Fo"'+$out+'/'+$name+'.obj"'
 if($name -like '*Catalog'){$compilerArgs+='/link';$compilerArgs+=($libs | ForEach-Object {'"'+$_+'"'})}
 [IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n");& "$out/build.cmd"
 if($LASTEXITCODE){throw "Native $name compilation failed"}
}
$previous=$env:PATH
try{
 $env:PATH="$repo/build-extractors/bin/Release;$env:PATH"
 $rows=@(& "$out/StormheimDrekirjarCatalog.exe" "$repo/build-extractors/bin/Release/dbc/enUS/" "$out/world.tsv");$code=$LASTEXITCODE;$rows | Set-Content "$out/native.tsv";$rows
 if($code -or $rows -notcontains 'area 7608 Hrydshal'){throw 'Native Drekirjar catalog failed'}
 & "$out/StormheimDrekirjarTest.exe" "$out/world.tsv" "$out/native.tsv"
 if($LASTEXITCODE){throw 'Native Drekirjar progression failed'}
}finally{$env:PATH=$previous}
