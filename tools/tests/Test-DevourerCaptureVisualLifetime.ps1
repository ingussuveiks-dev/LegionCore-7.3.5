param([switch]$Baseline,[switch]$AddressSanitizer)
$ErrorActionPreference='Stop'
if(!$Baseline){ & "$PSScriptRoot/Test-DevourerCaptureSequence.ps1" -AddressSanitizer:$AddressSanitizer; return }
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/devourer-capture-visual-lifetime"
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/$(if($Baseline){'baseline'}else{'current'})" -RuntimeDirectories @($out) -Test {
 $path='src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp'
 $source=Block ((& git -C $repo show "e4e4fe2f:$path") -join "`n") 'class npc_akaari_shadowgore :'
 $old=(& git -C $repo show "c0a7c900:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned capture visual baseline unavailable'};$old=Block $old 'class npc_akaari_shadowgore :'
 $callback=Block $source 'me->AddDelayedEvent(5000,';$oldCallback=Block $old 'me->AddDelayedEvent(5000,'
 if(($source.Replace($callback,$oldCallback)-replace '\s','') -cne ($old-replace '\s','')){throw 'Unexpected change outside original5s visual callback'}
 [IO.File]::WriteAllText("$out/Visual.inc",$(if($Baseline){$oldCallback}else{$callback})+');')
 $header=Get-Content "$repo/src/common/Utilities/FunctionProcessor.h" -Raw;$methods=Get-Content "$repo/src/common/Utilities/FunctionProcessor.cpp" -Raw
 [IO.File]::WriteAllText("$out/FunctionProcessor.inc",'typedef std::multimap<uint64, std::function<void()>> FunctionList;'+(Block $header 'class TC_COMMON_API FunctionProcessor').Replace('TC_COMMON_API ','')+';'+$methods.Substring($methods.IndexOf('FunctionProcessor::FunctionProcessor()')))
 $accessor=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.h" -Raw
 [IO.File]::WriteAllText("$out/Find.inc",(Block $accessor 'static T* Find(ObjectGuid guid)'))
 [IO.File]::WriteAllText("$out/Lookup.inc",(Block $accessor 'template<class T> static T* GetObjectInWorld(ObjectGuid guid, T*')+(Block $accessor 'template<class T> static T* GetObjectInMap(')+'static Creature* GetCreature(WorldObject const&,ObjectGuid);')
 [IO.File]::WriteAllText("$out/CreatureLookup.inc",(Block (Get-Content "$repo/src/server/game/Globals/ObjectAccessor.cpp" -Raw) 'Creature* ObjectAccessor::GetCreature('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $def=if($Baseline){'/DBASELINE'}else{''};if($AddressSanitizer){$def+=' /fsanitize=address /Zi'}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $def /I`"$out`" `"$PSScriptRoot/DevourerCaptureVisualLifetimeTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`"`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Capture visual lifetime regression failed'}
}
