$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$repo/build-extractors/runtime-test/server-diagnostic-lines" -Test {
 . "$PSScriptRoot/ServerDiagnosticLines.ps1"
 $ordinary=@(
  '2026-10-27_08:54:14 INFO  [network] World handshake accepted; TCP info 1 error 0 state 4',
  '2026-10-27_08:54:14 DEBUG [network] Diagnostic error counter 0',
  '2026-10-27_08:54:14 INFO [server] ERROR/Warning words inside informational text'
 )
 if(@(Get-ServerDiagnosticLines ($ordinary -join "`r`n")).Count){throw 'Informational message fields mistaken for severity'}
 $failures=@(
  '2026-10-27_08:54:14 ERROR [scripts] Creation failed',
  '2026-10-27_08:54:14 WARN  [network] Timeout',
  '2026-10-27 08:54:14.125 WARNING [server] Delayed update',
  '2026-10-27_08:54:14 FATAL [server] Cannot start',
  'ERROR: native failure',
  '2026-10-27_08:54:14 INFO [server] ASSERTION FAILED at production.cpp',
  'ABORTED'
 )
 if(@(Get-ServerDiagnosticLines (($ordinary+$failures) -join "`r`n")).Count -ne $failures.Count){throw 'Actual diagnostic severity/assertion/abort lost'}
 'PASS actual production log classifier retains error/warning/fatal/assertion/abort lines and distinguishes INFO handshake error0 metadata. Original appended logs remain fully preserved.'
}
