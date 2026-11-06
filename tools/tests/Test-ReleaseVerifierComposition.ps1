$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/release-verifier-composition/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
New-Item -ItemType Directory -Force $out|Out-Null
$python='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
[IO.File]::WriteAllText("$out/child.py",'raise SystemExit(0)')
[IO.File]::WriteAllText("$out/parent.py",("import runpy`nrunpy.run_path("+(ConvertTo-Json "$out/child.py" -Compress)+")`nprint('PARENT_COMPLETED')"))
[IO.File]::WriteAllText("$out/complete.py","print('PARENT_COMPLETED')")
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 & $python "$out/parent.py" *> "$out/counterfactual.log";$baseline=$LASTEXITCODE
 if($baseline -ne 0 -or ((Get-Content "$out/counterfactual.log" -Raw) -match 'PARENT_COMPLETED')){throw 'Expected nested-exit counterfactual changed'}
 'COUNTERFACTUAL nested SystemExit0 reports success while its parent never completes; expected broken baseline'
 & $python "$PSScriptRoot/run_linked_release_verifier.py" "$out/parent.py" *> "$out/rejected.log";$rejected=$LASTEXITCODE
 if($rejected -eq 0 -or !(Get-Content "$out/rejected.log" -Raw).Contains('Linked verifier exited before returning')){throw 'Nested exit was silently accepted'}
 & $python "$PSScriptRoot/run_linked_release_verifier.py" "$out/complete.py" *> "$out/complete.log"
 if($LASTEXITCODE -or !(Get-Content "$out/complete.log" -Raw).Contains('PASS linked verifier returned')){throw 'Ordinary returning verifier failed'}
 'PASS composed verifier rejects nested exit0, preserves its traceback, and accepts an actually completed parent'
}
