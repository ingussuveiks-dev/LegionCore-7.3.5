param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/devourer-outro-source/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
 $path='src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/instance_devourer.cpp';$old=(& git -C $repo show "e59f20dd3:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned instance absent'};$source=Get-Content "$repo/$path" -Raw
 $normalized=$source.Replace((Block $source 'void Update(uint32 diff)'),(Block $old 'void Update(uint32 diff)')).Replace('#include <algorithm>','')
 if(($old -replace '\s','') -cne ($normalized -replace '\s','')){throw 'Unreviewed change outside outro callback/include'}
 if($Baseline){$source=$old}
 $callbacks='';foreach($start in @('void Initialize()','void OnCreatureCreate(','void OnCreatureRemove(','void Update(uint32 diff)')){$callbacks+=(Block $source $start).Replace(' override','')+"`n"};[IO.File]::WriteAllText("$out/DevourerOutro.inc",$callbacks)
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $flag=if($Baseline){'/DDEVOURER_OUTRO_BASELINE'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd`"$out/compiler.pdb`" $flag /I`"$out`" `"$PSScriptRoot/DevourerOutroTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`nexit /b %errorlevel%`r`n")
 & "$out/run.cmd" *> "$out/source.log";if($LASTEXITCODE){Get-Content "$out/source.log" -Tail 8;throw 'Devourer outro source ASAN failed'};Get-Content "$out/source.log" -Tail 2
}
