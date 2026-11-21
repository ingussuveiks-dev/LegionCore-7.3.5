param([switch]$Baseline,[string]$OutputDirectory='',[string]$BaselineDirectory='',
      [switch]$TakingPrecautions,[switch]$Collector,[switch]$Scenes,[switch]$Kyle,[switch]$Seasoning,[switch]$SeasoningBaseline,
      [switch]$Aquantion,[switch]$AquantionBaseline,[switch]$Anowyn,[switch]$AnowynBaseline,
      [switch]$AlliedCity,[switch]$AlliedCityBaseline,[switch]$Duskwither,[switch]$DuskwitherBaseline,
      [switch]$RedSnapper,[switch]$RedSnapperBaseline,[switch]$SwiftDiscipline,[switch]$SwiftDisciplineBaseline,
      [switch]$ExtinguishingHope,[switch]$ExtinguishingHopeBaseline,
      [switch]$MulgoreCages,[switch]$MulgoreCagesBaseline,[string]$FixtureConfig='')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$scopedProbes = @(@($TakingPrecautions,$Collector,$Scenes,$Kyle,$Seasoning,$SeasoningBaseline,$Aquantion,$AquantionBaseline,$Anowyn,$AnowynBaseline,$AlliedCity,$AlliedCityBaseline,$Duskwither,$DuskwitherBaseline,$RedSnapper,$RedSnapperBaseline,$SwiftDiscipline,$SwiftDisciplineBaseline,$ExtinguishingHope,$ExtinguishingHopeBaseline,$MulgoreCages,$MulgoreCagesBaseline) | Where-Object { $_ }).Count
if($scopedProbes -and ($Baseline -or !$FixtureConfig -or $scopedProbes -gt 1)){throw 'Use one named probe and its private migrated fixture'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/quest-object-loot$(if($Baseline){'-baseline'}elseif($TakingPrecautions){'-taking-precautions'}elseif($Collector){'-collector'}elseif($Scenes){'-scenes'}elseif($Kyle){'-kyle'}elseif($Seasoning){'-seasoning'}elseif($SeasoningBaseline){'-seasoning-counterfactual'}elseif($Aquantion){'-aquantion'}elseif($AquantionBaseline){'-aquantion-counterfactual'}elseif($Anowyn){'-anowyn'}elseif($AnowynBaseline){'-anowyn-counterfactual'}elseif($AlliedCity){'-allied-city'}elseif($AlliedCityBaseline){'-allied-city-counterfactual'}elseif($Duskwither){'-duskwither'}elseif($DuskwitherBaseline){'-duskwither-counterfactual'}elseif($RedSnapper){'-red-snapper'}elseif($RedSnapperBaseline){'-red-snapper-counterfactual'}elseif($SwiftDiscipline){'-swift-discipline'}elseif($SwiftDisciplineBaseline){'-swift-discipline-counterfactual'}elseif($ExtinguishingHope){'-extinguishing-hope'}elseif($ExtinguishingHopeBaseline){'-extinguishing-hope-counterfactual'}elseif($MulgoreCages){'-mulgore-cages'}elseif($MulgoreCagesBaseline){'-mulgore-cages-counterfactual'})"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
[xml]$project=Get-Content "$repo/build-extractors/src/server/worldserver/worldserver.vcxproj"
$group=$project.Project.ItemDefinitionGroup|Where-Object Condition -like '*Release|x64*'|Select-Object -First 1
$includes=$group.ClCompile.AdditionalIncludeDirectories.Split(';')|Where-Object {$_ -and $_ -notlike '%*'}
$libs=$group.Link.AdditionalDependencies.Split(';')|Where-Object {$_ -and $_ -notlike '%*'}|ForEach-Object {if($_ -like '..*'){[IO.Path]::GetFullPath((Join-Path "$repo/build-extractors/src/server/worldserver" $_))}else{$_}}
$game=$libs|Where-Object {$_ -match 'game[\\/]Release[\\/]game.lib$'}|Select-Object -First 1
if(!$game){throw 'Missing actual final-link game library'}
if($Baseline){
 if(!$BaselineDirectory){$BaselineDirectory="$repo/.codex/quest-loot-baseline-330996949"}
 $manifest=Get-Content "$BaselineDirectory/manifest.json" -Raw|ConvertFrom-Json
 if($manifest.pin -ne '330996949' -or (Get-FileHash "$BaselineDirectory/game.lib").Hash -ne $manifest.game){throw 'Pinned counterfactual library hash differs'}
 $libs=$libs|ForEach-Object {if($_ -eq $game){"$BaselineDirectory/game.lib"}else{$_}}
 $game="$BaselineDirectory/game.lib"
}elseif(!$AlliedCityBaseline){
 foreach($file in @('Loot/LootMgr.cpp','Loot/LootMgr.h','Entities/Object/Object.h','Entities/Player/Player.cpp','Handlers/LootHandler.cpp')){
  if((Get-Item "$repo/src/server/game/$file").LastWriteTimeUtc -gt (Get-Item $game).LastWriteTimeUtc){throw "Actual game library predates $file"}
 }
}
$questSource=Get-Content "$repo/src/server/game/Globals/QuestData.cpp" -Raw
$query=[regex]::Match($questSource,'(?s)void QuestDataStoreMgr::LoadQuests\(\).*?WorldDatabase.Query\(("SELECT ".*?" FROM quest_template")\);').Groups[1].Value
if(!$query){throw 'Native quest loader SELECT missing'}
[IO.File]::WriteAllText("$out/NativeLostArmyQuest.inc",$query+' " WHERE ID IN(44923,45088)"')
$probeSource = "$PSScriptRoot/QuestObjectLootNative.cpp"
if($TakingPrecautions){
 $probeSource = "$PSScriptRoot/HallowsEndTakingPrecautionsNative.cpp"
 [IO.File]::WriteAllText("$out/NativeTakingPrecautionsQuest.inc",$query+' " WHERE ID IN(29402,29426)"')
}
if($Collector){
 $probeSource = "$PSScriptRoot/HallowsEndCollectorNative.cpp"
 [IO.File]::WriteAllText("$out/NativeCollectorQuest.inc",$query+' " WHERE ID IN(29403,29411,29413,29427,29428,29429)"')
}
if($Scenes){
 $probeSource = "$PSScriptRoot/HallowsEndScenesNative.cpp"
 [IO.File]::WriteAllText("$out/NativeHallowsEndScenesQuest.inc",$query+' " WHERE ID IN(29399,29411,29425,29428)"')
}
if($Kyle){
 $probeSource = "$PSScriptRoot/KyleLunchNative.cpp"
 [IO.File]::WriteAllText("$out/NativeKyleQuest.inc",$query+' " WHERE ID=11129"')
}
if($Seasoning -or $SeasoningBaseline){
 $probeSource = "$PSScriptRoot/SeasoningRacksNative.cpp"
 [IO.File]::WriteAllText("$out/NativeSeasoningQuest.inc",$query+' " WHERE ID=9275"')
}
if($Aquantion -or $AquantionBaseline){
 $probeSource = "$PSScriptRoot/AquantionAltarNative.cpp"
 [IO.File]::WriteAllText("$out/NativeAquantionQuest.inc",$query+' " WHERE ID=9174"')
}
if($Anowyn -or $AnowynBaseline){
 $probeSource = "$PSScriptRoot/AnowynCrystalNative.cpp"
 [IO.File]::WriteAllText("$out/NativeAnowynQuest.inc",$query+' " WHERE ID=9169"')
}
if($AlliedCity -or $AlliedCityBaseline){
 $probeSource = "$PSScriptRoot/AlliedRaceCityBoardNative.cpp"
 [IO.File]::WriteAllText("$out/NativeAlliedCityQuest.inc",$query+' " WHERE ID IN (49772,49773,49788,49933,50303,50305,50313,50319)"')
}
if($Duskwither -or $DuskwitherBaseline){
 $probeSource = "$PSScriptRoot/DuskwitherSpireNative.cpp"
 [IO.File]::WriteAllText("$out/NativeDuskwitherQuest.inc",$query+' " WHERE ID=8889"')
}
if($RedSnapper -or $RedSnapperBaseline){
 $probeSource = "$PSScriptRoot/RedSnapperNative.cpp"
 [IO.File]::WriteAllText("$out/NativeRedSnapperQuest.inc",$query+' " WHERE ID=9452"')
}
if($SwiftDiscipline -or $SwiftDisciplineBaseline){
 $probeSource = "$PSScriptRoot/SwiftDisciplineNative.cpp"
 [IO.File]::WriteAllText("$out/NativeSwiftDisciplineQuest.inc",$query+' " WHERE ID=9066"')
}
if($ExtinguishingHope -or $ExtinguishingHopeBaseline){
 $probeSource = "$PSScriptRoot/ExtinguishingHopeNative.cpp"
 [IO.File]::WriteAllText("$out/NativeExtinguishingHopeQuest.inc",$query+' " WHERE ID IN (26391,28817,28818,28819,28820,28821,28822,28823,29083,31145)"')
}
if($MulgoreCages -or $MulgoreCagesBaseline){
 $probeSource = "$PSScriptRoot/MulgoreCagesNative.cpp"
 [IO.File]::WriteAllText("$out/NativeMulgoreCagesQuest.inc",$query+' " WHERE ID IN (14452,24852,14458)"')
}
$extra=@('src/server/scripts/Legion','src/common/Collision/Management','src/common/Collision','dep/g3dlite/include','dep/recastnavigation/Detour/Include')|ForEach-Object {Join-Path $repo $_}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$language=switch($group.ClCompile.LanguageStandard){'stdcpp14'{'/std:c++14'}'stdcpp17'{'/std:c++17'}'stdcpp20'{'/std:c++20'}'stdcpplatest'{'/std:c++latest'}default{throw 'Unsupported native language'}}
if($group.ClCompile.RuntimeLibrary -ne 'MultiThreadedDLL'){throw 'Expected actual Release /MD ABI'}
$compilerArgs=@('/nologo','/EHsc',$language,'/MD','/DNDEBUG','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/D_WIN32_WINNT=0x0601','/DBOOST_ALL_NO_LIB')
if($Baseline){$compilerArgs+='/DQUEST_OBJECT_LOOT_BASELINE'}
if($SeasoningBaseline){$compilerArgs+='/DSEASONING_UNGUARDED_BASELINE'}
if($AquantionBaseline){$compilerArgs+='/DAQUANTION_UNGUARDED_BASELINE'}
if($AnowynBaseline){$compilerArgs+='/DANOWYN_UNGUARDED_BASELINE'}
if($AlliedCityBaseline){$compilerArgs+='/DALLIED_CITY_BASELINE'}
if($DuskwitherBaseline){$compilerArgs+='/DDUSKWITHER_BASELINE'}
if($RedSnapperBaseline){$compilerArgs+='/DRED_SNAPPER_BASELINE'}
if($SwiftDisciplineBaseline){$compilerArgs+='/DSWIFT_DISCIPLINE_BASELINE'}
if($ExtinguishingHopeBaseline){$compilerArgs+='/DEXTINGUISHING_HOPE_BASELINE'}
if($MulgoreCagesBaseline){$compilerArgs+='/DMULGORE_CAGES_BASELINE'}
$compilerArgs+=($includes+$extra+@('C:/local/boost_1_85_0','C:/wamp64/bin/mysql/mysql5.7.44/include','C:/Program Files/OpenSSL-Win64/include',$out)|ForEach-Object {'/I"'+$_+'"'})
$compilerArgs+='"'+$probeSource+'"'
$compilerArgs+=@('/Fe"'+$out+'/probe.exe"','/Fo"'+$out+'/probe.obj"','/link','/DEBUG','/MAP:"'+$out+'/probe.map"','/PDB:"'+$out+'/probe.pdb"')
$compilerArgs+=($libs|ForEach-Object {'"'+$_+'"'})
[IO.File]::WriteAllText("$out/build.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl "+($compilerArgs -join ' ')+"`nexit /b %errorlevel%`n")
& "$out/build.cmd";if($LASTEXITCODE){throw 'Native quest-object loot build failed'}
$runtime="$repo/build-extractors/bin/Release"
$run="$out/run-$(Get-Date -Format yyyyMMdd-HHmmss)";New-Item -ItemType Directory -Force $run|Out-Null
$config=Get-Content -LiteralPath $(if($FixtureConfig){$FixtureConfig}else{"$runtime/worldserver.conf"}) -Raw
$config=[regex]::Replace($config,'(?m)^LogsDir\s*=.*$',('LogsDir = "'+$run.Replace('\','/')+'"'))
[IO.File]::WriteAllText("$run/worldserver.conf",$config)
$prior=$env:PATH
try{
 $env:PATH="$runtime;$prior";Push-Location $runtime
 try{& "$out/probe.exe" "$run/worldserver.conf" "$runtime/" > "$run/stdout.log" 2> "$run/stderr.log";$exit=$LASTEXITCODE}finally{Pop-Location}
}finally{$env:PATH=$prior}
Get-Content "$run/stdout.log"|Where-Object {$_ -match '^(PASS|EXPECTED_BROKEN|CASE)'}
if($exit -or (Get-Item "$run/stderr.log").Length){Get-Content "$run/stderr.log" -Tail 12;throw "Native quest-object loot exit=$exit or stderr"}
if((Test-Path "$run/DBErrors.log") -and (Get-Item "$run/DBErrors.log").Length){throw 'Native loot emitted DBErrors; inspect preserved evidence'}
@{baseline=[bool]($Baseline -or $SeasoningBaseline -or $AquantionBaseline -or $AnowynBaseline -or $AlliedCityBaseline -or $DuskwitherBaseline -or $RedSnapperBaseline -or $SwiftDisciplineBaseline -or $ExtinguishingHopeBaseline -or $MulgoreCagesBaseline);game=(Get-FileHash $game).Hash;executable=(Get-FileHash "$out/probe.exe").Hash;source=(Get-FileHash $probeSource).Hash;run=$run;exit=$exit}|ConvertTo-Json|Set-Content "$out/native-inputs.json"
'PASS native execution/exit/stdout/stderr/DBErrors; wrapper must also inspect crash and appended server diagnostics'
