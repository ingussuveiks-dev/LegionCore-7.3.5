param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/kingslayers-gate-order-asan/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $path='src/server/scripts/Scenario/Artifacts/Rogue/Kingslayers/Kingslayers.cpp';$source=Get-Content "$repo/$path" -Raw;$old=(& git -C $repo show "ff238f715:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned gate predecessor missing'}
 $marker='class npc_kingslayers_small_events : public CreatureScript';$body=Block $source $marker;$prior=Block $old $marker
 $opening='struct npc_garona_halforcenAI : ScriptedAI'
 $normalized=$source.Replace($body,$prior).Replace((Block $source $opening),(Block $old $opening))
 if(($normalized -replace '\s','') -cne ($old -replace '\s','')){throw 'Only gate AI and opening bridge may change'}
 # Certify every pre-existing shared small-events statement after subtracting
 # the independently exercised scoped controller and its two dispatch points.
 $shared=$body.Substring(0,$body.IndexOf('        ObjectGuid gateOrderRecipient;'))+$body.Substring($body.IndexOf('        void MoveInLineOfSight(Unit* who) override'))
 $scoped=Block $shared 'if (me->GetEntry() == 108057 && me->GetMapId() == 1620)'
 $shared=$shared.Replace($scoped,'').Replace('            UpdateGateOrder(diff);','')
 if(($shared -replace '\s','') -cne ($prior -replace '\s','')){throw 'Unreviewed change in original shared actor/combat/scene bodies'}
 [IO.File]::WriteAllText("$out/SmallAI.inc",(Block $body 'struct npc_kingslayers_small_eventsAI : ScriptedAI')+';')
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/KingslayersGateOrderTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b %errorlevel%`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd" > "$out/stdout.log" 2> "$out/stderr.log";$code=$LASTEXITCODE
 Get-Content "$out/stdout.log";if($code -or (Get-Item "$out/stderr.log").Length){Get-Content "$out/stderr.log" -Tail 20;throw "Gate ASAN exit=$code or stderr"}
}
