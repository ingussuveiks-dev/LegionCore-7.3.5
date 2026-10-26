param([switch]$Baseline,[switch]$AddressSanitizer,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'A counterfactual cannot certify the repaired executable'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/kingslayers-intro-lifetime"
New-Item -ItemType Directory -Force $out|Out-Null
if(!$ReleaseDirectory){$ReleaseDirectory="$repo/build-extractors/bin/Release"}
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
function Crash-State {
 $state=@{}
 foreach($directory in @("$repo/build-extractors/bin/Release/Crashes","$out/Crashes")){
  foreach($file in @(Get-ChildItem -LiteralPath $directory -File -ErrorAction SilentlyContinue)){$state[$file.FullName]=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash}
 }
 return $state
}
function Read-Tail([string]$path,[long]$offset){
 if(!(Test-Path -LiteralPath $path)){return ''}
 $stream=[IO.File]::Open($path,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
 try{if($stream.Length -lt $offset){$offset=0};[void]$stream.Seek($offset,[IO.SeekOrigin]::Begin);$reader=New-Object IO.StreamReader($stream);try{$reader.ReadToEnd()}finally{$reader.Dispose()}}finally{$stream.Dispose()}
}
$beforeCrashes=Crash-State
$logs=@{}
foreach($name in @('DBErrors.log','Server.log')){$path="$repo/build-extractors/bin/Release/logs/$name";$logs[$path]=if(Test-Path -LiteralPath $path){(Get-Item -LiteralPath $path).Length}else{0}}
$completed=$false
try{
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp'
 $source=Get-Content "$repo/$path" -Raw -Encoding UTF8
 $pinned=(& git -C $repo show "3eadb186:$path") -join "`n"
 if($LASTEXITCODE){throw 'Pinned Kingslayers baseline unavailable'}
 $marker='struct npc_garona_halforcenAI : ScriptedAI'
 $restored=$source.Replace((Block $source $marker),(Block $pinned $marker)).Replace('#include "ObjectAccessor.h"','')
 $merlis='struct npc_merlis_malaganAI : ScriptedAI'
 $restored=$restored.Replace((Block $source $merlis),(Block $pinned $merlis)) # Independently pinned by Test-KingslayersDeathCredit.
 if(($restored -replace '\s','') -cne ($pinned -replace '\s','')){throw 'Unreviewed change outside Garona introduction AI'}
 if($Baseline){$source=$pinned}
 [IO.File]::WriteAllText("$out/GaronaIntro.inc",(Block $source $marker)+';')
 $header=Get-Content "$repo/src/server/game/AI/EventMap.h" -Raw -Encoding UTF8
 $methods=Get-Content "$repo/src/server/game/AI/EventMap.cpp" -Raw -Encoding UTF8
 [IO.File]::WriteAllText("$out/EventMapClass.inc",(Block $header 'enum c_events')+';'+(Block $header 'class TC_GAME_API EventMap').Replace('TC_GAME_API ','')+';')
 [IO.File]::WriteAllText("$out/EventMapMethods.inc",((@('void EventMap::Reset(','void EventMap::ScheduleEvent(uint32 eventId, uint32 time','uint32 EventMap::ExecuteEvent(','void EventMap::CancelEvent(')|ForEach-Object {Block $methods $_}) -join "`n"))
 $header=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw -Encoding UTF8
 $methods=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw -Encoding UTF8
 [IO.File]::WriteAllText("$out/FunctionProcessor.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $header 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$methods.Substring($methods.IndexOf('FunctionProcessor::FunctionProcessor()')))
 $accessor=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.h" -Raw -Encoding UTF8
 [IO.File]::WriteAllText("$out/PlayerFind.inc",(Block $accessor 'static T* Find(ObjectGuid guid)'))
 [IO.File]::WriteAllText("$out/PlayerLookup.inc",(Block $accessor 'static Player* GetObjectInWorld(ObjectGuid guid, Player*')+(Block $accessor 'template<class T> static T* GetObjectInMap(')+'static Player* GetPlayer(WorldObject const&, ObjectGuid);')
 [IO.File]::WriteAllText("$out/PlayerLookupMethod.inc",(Block (Get-Content "$repo/src/server/game/Globals/ObjectAccessor.cpp" -Raw -Encoding UTF8) 'Player* ObjectAccessor::GetPlayer('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $definition=if($Baseline){'/DBASELINE'}else{''}
 if($AddressSanitizer){$definition+=' /fsanitize=address /Zi'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/KingslayersIntroLifetimeTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd" *> "$out/native.log"
 Get-Content "$out/native.log" -Encoding UTF8
 if($LASTEXITCODE){throw 'Kingslayers native lifecycle regression failed'}
 if(!$Baseline){
  $previous=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile KingslayersIntroCatalog.cpp -ProbeName KingslayersIntroCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Kingslayers native catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
  $expected=@("MAP`t1620`t1`t1","STEP`t0`t2548`t48525`t0","STEP`t1`t2549`t48530`t0","STEP`t2`t2550`t48532`t0","STEP`t3`t2711`t49470`t0","STEP`t4`t2558`t48576`t0","STEP`t5`t2560`t48584`t0","STEP`t6`t2561`t48586`t0","STEP`t7`t2562`t48588`t0","STEP`t8`t2563`t48590`t0","STEP`t9`t2564`t48592`t0","CRITERION`t48526`t48525`t30443`t92`t51418","CRITERION`t49374`t48525`t30887`t92`t52672")
  if(Compare-Object $expected (Get-Content "$out/native.tsv" -Encoding UTF8)){throw 'Native1123 steps or personal event criteria differ'}
  'PASS native26972 base/hotfix map1620 and1123 ten-step catalog;51418/52672 are stage0 type92 criteria'
 }
 if($VerifyReleaseBinary){
  $previous=$env:PYTHONPATH
  try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_kingslayers_intro_release.py" $ReleaseDirectory;if($LASTEXITCODE){throw 'Selected Kingslayers Release implementation missing'}}finally{$env:PYTHONPATH=$previous}
 }
 $completed=$true
}finally{
 $afterCrashes=Crash-State
 $changed=@($afterCrashes.Keys|Where-Object {!$beforeCrashes.ContainsKey($_) -or $beforeCrashes[$_] -cne $afterCrashes[$_]})
 $diagnostics=@();$newDb=''
 foreach($path in $logs.Keys){$tail=Read-Tail $path $logs[$path];[IO.File]::WriteAllText("$out/appended-$([IO.Path]::GetFileName($path))",$tail);if($path.EndsWith('/DBErrors.log')){$newDb=$tail};$diagnostics+=@($tail -split "`n"|Where-Object {$_ -match '(?i)\b(ERROR|WARN(?:ING)?|FATAL)\b|ASSERTION FAILED|ABORTED'})}
 $result=[ordered]@{Completed=$completed;Baseline=[bool]$Baseline;AddressSanitizer=[bool]$AddressSanitizer;ChangedCrashes=$changed;NewDBErrorsCharacters=$newDb.Length;NewServerDiagnostics=$diagnostics;ActiveUserServers=@(Get-Process worldserver,bnetserver -ErrorAction SilentlyContinue|Select-Object Id,ProcessName,StartTime)}
 [IO.File]::WriteAllText("$out/post-test-diagnostics.json",($result|ConvertTo-Json -Depth 5))
 if($changed.Count -or $newDb.Length -or $diagnostics.Count){throw "Post-test diagnostics need investigation; see $out/post-test-diagnostics.json"}
 'PASS post-test canonical/fixture crashes unchanged; no appended DBErrors or server warnings/errors/assertions; no server stopped or log cleared'
}
