param([switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/.codex/warswords-runestone-model-20261113/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/prepare_warswords_runestone.py" $out
 if($LASTEXITCODE){throw 'Runestone source extraction failed'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/test.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /MD /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/WarswordsRunestoneTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/compiler.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & cmd.exe /d /c "`"$out/test.cmd`" > `"$out/test.log`" 2>&1"
 if($LASTEXITCODE){Get-Content "$out/test.log" -Tail 16;throw 'Runestone controlled ASAN regression failed'}
 Get-Content "$out/test.log" -Tail 1
 if($VerifyNativeCatalog){
  $previousConfig=$env:LEGION_DIALOGUE_CONFIG
  try{
   $env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf"
   & "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarswordsRunestoneCatalog.cpp -ProbeName WarswordsRunestoneCatalog -OutputFile "$out/catalog.tsv" *> "$out/catalog.log"
   if($LASTEXITCODE){throw 'Native runestone data reader failed'}
  }finally{$env:LEGION_DIALOGUE_CONFIG=$previousConfig}
  $rows=Get-Content "$out/catalog.tsv"
  foreach($row in @("LOCK`t99`t1`t2`t14`t0","SPELL`t8386`tAttacking","EFFECT`t8386`t0`t0`t33`t14`t23`t0","MISC`t8386`t0`t4`t1000","STEP`t1891`t45862","CRITERIA`t45863`t29081`t92`t46970`t4","OTHER_QUEST_GO_OBJECTIVES`t0","EVENT_USE`t944`t1891","BINDING`t244940`t10")){if($rows -notcontains $row){throw "Matching native rune data differs:$row"}}
  if(@($rows|Where-Object {$_ -match '^(EVENT_USE|BINDING)\s'}).Count -ne 2){throw 'Additional shared native rune usage needs review'}
  foreach($guid in 134130..134133){if($rows -notcontains "ROW`t$guid`t244940`t1511`t4096`t1`t7003`t1"){throw 'Actual four-spawn rune context differs'}}
  'PASS matching26972 Lock99/Attacking8386/1000ms,194802 native presentation and four actual1511 runestones/tree45863/event46970 amount4'
 }
}
