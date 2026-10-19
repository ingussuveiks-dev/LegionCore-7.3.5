$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../..").Path
$output=Join-Path $repo 'build-extractors/runtime-test/spell-between-lifetime'
New-Item -ItemType Directory -Force $output | Out-Null
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($baseline in @($false,$true)){
 $directory=if($baseline){Join-Path $output 'baseline'}else{Join-Path $output 'current'}
 New-Item -ItemType Directory -Force $directory | Out-Null
 if($baseline){
  $lines=& git -C $repo show 'de86651c:src/server/game/Spells/Spell.cpp'
  if($LASTEXITCODE){throw 'Pinned native lifetime baseline unavailable'}
  $source=$lines -join "`n"
 }else{$source=Get-Content "$repo/src/server/game/Spells/Spell.cpp" -Raw}
 $function=$source.IndexOf('void Spell::SelectImplicitBetweenTargets(')
 $end=$source.IndexOf('        Trinity::WorldObjectSpellBetweenTargetCheck check(',$function)
 $start=$source.IndexOf('        Position nearDestination;',$function)
 if($start -lt 0 -or $start -gt $end){$start=$source.IndexOf('        Position const* center = nullptr;',$function)}
 if($function -lt 0 -or $start -lt 0 -or $end -le $start){throw 'Native between-selection lifetime block not found'}
 [IO.File]::WriteAllText("$directory/BetweenSelection.inc",$source.Substring($start,$end-$start))
 $define=if($baseline){'/DBETWEEN_LIFETIME_BASELINE'}else{''}
 $command="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /MD $define /I`"$directory`" `"$repo/tools/tests/SpellBetweenLifetimeTest.cpp`" /Fe`"$directory/test.exe`" /Fo`"$directory/test.obj`"`nexit /b %errorlevel%`n"
 [IO.File]::WriteAllText("$directory/build.cmd",$command)
 & "$directory/build.cmd"
 if($LASTEXITCODE){throw 'Native between-selection lifetime test did not compile'}
 $rows=& "$directory/test.exe"
 $result=$LASTEXITCODE
 $rows | Set-Content "$directory/results.txt"
 $rows
 if($result){throw 'Native between-selection Position lifetime regression failed'}
}
'PASS: lifetime instrumentation checks the exact extracted native selection scope before dereferencing its center; native geometry/combat remains covered separately by the Rockaway offensive probe.'
