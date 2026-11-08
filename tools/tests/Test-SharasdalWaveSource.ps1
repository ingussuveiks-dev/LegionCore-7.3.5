param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/sharasdal-wave-source/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 . "$PSScriptRoot/SharasdalWaveScope.ps1"
 $path='src/server/scripts/Scenario/Artifacts/Shaman/Sharasdal/Sharasdal.cpp';$old=(& git -C $repo show "dbb95ce8d:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned whole Sharasdal absent'};$source=Get-Content "$repo/$path" -Raw
 if(((Remove-SharasdalWaveScope $source) -replace '\s','') -cne ($old -replace '\s','')){throw 'Unreviewed change outside wave cleanup'}
 $once=Remove-SharasdalWaveScope $source;if((Remove-SharasdalWaveScope $once) -cne $once){throw 'Repeated wave certificate changed source'}
 $refused=$false;try{Remove-SharasdalWaveScope ($source.Replace('me->DespawnOrUnsummon(100);','me->DespawnOrUnsummon(101);'))|Out-Null}catch{if($_ -notmatch 'Uncertified later wave cleanup change'){throw};$refused=$true};if(!$refused){throw 'Unknown wave delay accepted'}
 [IO.File]::WriteAllText("$out/SharasdalWave.inc",(Read-SharasdalWaveBlock $(if($Baseline){$old}else{$source}))+';')
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $mode=if($Baseline){'/DSHARASDAL_WAVE_BASELINE'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd`"$out/compiler.pdb`" $mode /I`"$out`" `"$PSScriptRoot/SharasdalWaveTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`nexit /b %errorlevel%`r`n")
 & "$out/run.cmd" *> "$out/source.log";if($LASTEXITCODE){Get-Content "$out/source.log" -Tail 8;throw 'Wave source ASAN failed'};Get-Content "$out/source.log" -Tail 2
}
