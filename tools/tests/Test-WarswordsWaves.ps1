param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-waves/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 Copy-Item "$repo/src/server/scripts/Scenario/Artifacts/Warrior/Warswords/warswords_waves.h" "$out/Waves.inc"
 $legacy=(& git -C $repo show 'd473ec251:src/server/scripts/Scenario/Artifacts/Warrior/Warswords/instance_warswords.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned wave baseline unavailable'}
 $a=$legacy.IndexOf('        void DoSummonAdds(');$b=$legacy.IndexOf('        void OnCreatureCreate(',$a);if($a -lt 0 -or $b -le $a){throw 'Pinned native wave method missing'}
 [IO.File]::WriteAllText("$out/Legacy.inc",$legacy.Substring($a,$b-$a))
 $source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Warrior/Warswords/instance_warswords.cpp" -Raw
 foreach($fragment in @('waves(map)','waves.Reset();','waves.Start();','waves.Death(creature);','waves.Update(diff);')){if(!$source.Contains($fragment)){throw "Missing actual instance hook:$fragment"}}
 if($source.Contains('DoSummonAdds') -or $source.Contains('count++')){throw 'Legacy per-player/untracked death counter remains'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/WarswordsWavesTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd" *> "$out/test.log";if($LASTEXITCODE){Get-Content "$out/test.log" -Tail 15;throw 'Fury wave source regression failed'}
 Get-Content "$out/test.log"|Where-Object {$_ -match '^(PASS|COUNTERFACTUAL)'}
 $old=$env:LEGION_DIALOGUE_CONFIG
 try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarswordsCreditCatalog.cpp -ProbeName WarswordsCreditCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native wave catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$old}
 $native=Get-Content "$out/native.tsv"
 foreach($row in @("CRITERION`t1`t29055`t92`t46804`t1","CRITERION`t1`t29100`t92`t47289`t3")){if($row -notin $native){throw "Missing native wave criterion:$row"}}
 'PASS matching26972 stage1: three47289 wave events plus separate real46804 leader death; no invented kill quantity.'
}
