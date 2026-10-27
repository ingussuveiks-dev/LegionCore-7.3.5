param([switch]$AddressSanitizer,[switch]$VerifyNativeCatalog)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/bilaal-combat"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing native block $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){if($s[$b] -eq '{'){++$depth};if($s[$b] -eq '}'){--$depth};++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($repo,$out,"$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScript.cpp" -Raw
 $a=$s.IndexOf('        case SMART_ACTION_CAST:');$b=$s.IndexOf('        case SMART_ACTION_CAST_CUSTOM:',$a)
 [IO.File]::WriteAllText("$out/CastAction.inc",$s.Substring($a,$b-$a))
 if($s.IndexOf('e.runOnce = true;') -gt $a){throw 'Native pre-dispatch latch placement changed'}
 $event=Block $s 'void SmartScript::ProcessEvent('
 $a=$event.IndexOf('{')+1;$b=$event.IndexOf('    switch (e.GetEventType())')
 $prefix=$event.Substring($a,$b-$a)
 $a=$event.IndexOf('        case SMART_EVENT_HEALTH_PCT:');$b=$event.IndexOf('        case SMART_EVENT_TARGET_HEALTH_PCT:',$a)
 [IO.File]::WriteAllText("$out/HealthEvent.inc",$prefix+"switch(e.GetEventType()){`n"+$event.Substring($a,$b-$a)+"default:return;}`n")
 [IO.File]::WriteAllText("$out/Reset.inc",(Block $s 'void SmartScript::OnReset()'))
 [IO.File]::WriteAllText("$out/UpdateTimer.inc",(Block $s 'void SmartScript::UpdateTimer('))
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.h" -Raw
 [IO.File]::WriteAllText("$out/CastFlags.inc",(Block $s 'enum SmartCastFlags')+';')
 $enums='';foreach($name in @('SMART_EVENT','SMART_ACTION','SMARTAI_TARGETS','SmartScriptType','SmartEventFlags')){$enums+=(Block $s "enum $name`r`n")+';'}
 [IO.File]::WriteAllText("$out/NativeEnums.inc",$enums)
 $s=Get-Content "$repo/src/server/game/AI/SmartScripts/SmartScriptMgr.cpp" -Raw
 $a=$s.IndexOf('        case SMART_ACTION_ADD_AURA:');$b=$s.IndexOf('        case SMART_ACTION_CALL_AREAEXPLOREDOREVENTHAPPENS:',$a)
 [IO.File]::WriteAllText("$out/Validation.inc",$s.Substring($a,$b-$a))
 $native=Get-Content "$PSScriptRoot/BilaalCombatOriginal.json" -Raw|ConvertFrom-Json
 $rows=@($native.rows|Where-Object {$_.event_type -eq 2 -and $_.action_type -eq 11 -and $_.action_param1 -eq 209863})
 if($rows.Count -ne 6){throw 'Missing six actual native threshold rows'}
 [IO.File]::WriteAllText("$out/native-shapes.tsv",(($rows|ForEach-Object {"$($_.entryorguid)`t$($_.id)`t$($_.event_param2)"}) -join "`n")+"`n")
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $asan=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`ncl /nologo /EHsc /std:c++17 /W4 $asan /I`"$out`" `"$PSScriptRoot/BilaalCombatTest.cpp`" /Fe:test.exe /Fo:test.obj`nif errorlevel 1 exit /b 1`ntest.exe native-shapes.tsv`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Bilaal health-threshold regression failed'}
 if($VerifyNativeCatalog){
  $prior=$env:LEGION_DIALOGUE_CONFIG
  try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile LightsWrathCatalog.cpp -ProbeName LightsWrathCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$prior}
  $catalog=@(Get-Content "$out/native.tsv")
  foreach($row in @("SPELL`t209863`tForce of Gravity","EFFECT`t209863`t0`t6`t23`t0`t0`t1`t0","DURATION`t209863`t15000`t15000","LFG`t1228`t1065`t1583`t12","LFG`t1241`t1101`t1583`t12")){if($catalog -notcontains $row){throw 'Native Bilaal/shared scenario contract changed'}}
 }
}
