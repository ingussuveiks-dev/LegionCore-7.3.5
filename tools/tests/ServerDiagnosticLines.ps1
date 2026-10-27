function Get-ServerDiagnosticLines([string]$Text) {
 @($Text -split "`n" | Where-Object {
  # Classify the log severity, not message fields such as TCP "error 0".
  $_ -match '^\s*(?:\d{4}-\d{2}-\d{2}[_ ]\d{2}:\d{2}:\d{2}(?:\.\d+)?\s+)?(?:ERROR|WARN(?:ING)?|FATAL)\b' -or
  $_ -match 'ASSERTION FAILED|ABORTED'
 })
}
