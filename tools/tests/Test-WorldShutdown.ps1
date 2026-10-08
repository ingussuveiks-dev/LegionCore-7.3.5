param(
    [string]$Runtime = "$PSScriptRoot/../../build-extractors/bin/Release",
    [switch]$EndOfInput,
    [switch]$ProbeConnections,
    [switch]$CloseInputAfterCommand,
    [switch]$Utf8Bom
)
$ErrorActionPreference = 'Stop'
$Runtime = (Resolve-Path $Runtime).Path
if (Get-Process worldserver -ErrorAction SilentlyContinue) { throw 'Stop worldserver before running this test.' }
$before = @(Get-ChildItem "$Runtime/Crashes" -Filter '*.dmp' -ErrorAction SilentlyContinue).Count
$serverLogPath = Join-Path $Runtime 'logs/Server.log'
$existingLog = Get-Item -LiteralPath $serverLogPath -ErrorAction SilentlyContinue
$logOffset = if ($existingLog) { $existingLog.Length } else { 0 }
$newLog = [Text.StringBuilder]::new()
$started = Get-Date
$info = [Diagnostics.ProcessStartInfo]::new()
$info.FileName = Join-Path $Runtime 'worldserver.exe'
$info.WorkingDirectory = $Runtime
$info.UseShellExecute = $false
$info.CreateNoWindow = $true
$info.RedirectStandardInput = $true
$info.RedirectStandardOutput = $true
$info.RedirectStandardError = $true
$process = [Diagnostics.Process]::new()
$process.StartInfo = $info
# .NET Framework constructs an AutoFlush StreamWriter during Start(), which
# can emit Console.InputEncoding's preamble even when we later use BaseStream.
$previousInputEncoding = [Console]::InputEncoding
try {
    [Console]::InputEncoding = [Text.UTF8Encoding]::new($false)
    [void]$process.Start()
}
finally { [Console]::InputEncoding = $previousInputEncoding }
$stdout = $process.StandardOutput.ReadToEndAsync()
$stderr = $process.StandardError.ReadToEndAsync()
try {
    if ($EndOfInput) { $process.StandardInput.Close() }
    else {
        $ready = $false
        $deadline = $started.AddSeconds(90)
        while (!$process.HasExited -and (Get-Date) -lt $deadline) {
            if (Test-Path -LiteralPath $serverLogPath) {
                $stream = [IO.File]::Open($serverLogPath, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)
                try {
                    if ($stream.Length -lt $logOffset) { $logOffset = 0; [void]$newLog.Clear() }
                    [void]$stream.Seek($logOffset, [IO.SeekOrigin]::Begin)
                    $reader = [IO.StreamReader]::new($stream)
                    try { [void]$newLog.Append($reader.ReadToEnd()); $logOffset = $stream.Position }
                    finally { $reader.Dispose() }
                }
                finally { $stream.Dispose() }
            }
            # Existing logs contain ready lines from earlier server runs.
            # Only this process's appended output can establish readiness.
            if ($newLog.ToString().Contains('(worldserver-daemon) ready...')) {
                $ready = $true
                break
            }
            Start-Sleep -Milliseconds 250
        }
        if (!$ready) { throw 'Server did not reach ready within 90 seconds.' }
        if ($ProbeConnections) {
            foreach ($setting in @('WorldServerPort', 'InstanceServerPort')) {
                $line = (Select-String "$Runtime/worldserver.conf" -Pattern "^$setting\s*=").Line
                $port = [int](($line -split '=', 2)[1].Trim())
                for ($attempt = 0; $attempt -lt 3; $attempt++) {
                    $client = [Net.Sockets.TcpClient]::new()
                    try {
                        $client.Connect('127.0.0.1', $port)
                        $client.ReceiveTimeout = 3000
                        if ($client.GetStream().ReadByte() -lt 0) { throw "No server greeting on port $port" }
                    }
                    finally { $client.Dispose() }
                }
            }
        }
        # Write exact bytes: .NET Framework's default StreamWriter may silently
        # emit a BOM. Cover both inputs explicitly and keep stdin OPEN.
        $bytes = [Text.Encoding]::UTF8.GetBytes("server shutdown 0`r`n")
        if ($Utf8Bom) { $bytes = [byte[]](0xEF, 0xBB, 0xBF) + $bytes }
        $process.StandardInput.BaseStream.Write($bytes, 0, $bytes.Length)
        $process.StandardInput.BaseStream.Flush()
        if ($CloseInputAfterCommand) { $process.StandardInput.Close() }
    }
    if (!$process.WaitForExit(60000)) {
        $process.StandardInput.Close()
        if ($process.WaitForExit(30000)) {
            $diagnostic = $stdout.GetAwaiter().GetResult()
            Write-Output ($diagnostic.Substring([Math]::Max(0, $diagnostic.Length - 2500)))
        }
        throw 'Server did not finish shutdown within 60 seconds with stdin open.'
    }
    $out = $stdout.GetAwaiter().GetResult()
    $err = $stderr.GetAwaiter().GetResult()
    $after = @(Get-ChildItem "$Runtime/Crashes" -Filter '*.dmp' -ErrorAction SilentlyContinue).Count
    if ($process.ExitCode -ne 0 -or $after -ne $before -or !$out.Contains('(worldserver-daemon) ready...') -or
        $out.Contains('There is no such command')) {
        throw "Shutdown failed: exit=$($process.ExitCode), new dumps=$($after-$before). $err"
    }
    "PASS: EndOfInput=$EndOfInput; Utf8Bom=$Utf8Bom; ProbeConnections=$ProbeConnections; CloseInputAfterCommand=$CloseInputAfterCommand; ready reached; exit=0; new crash dumps=0"
}
finally {
    if (!$process.HasExited) {
        $process.StandardInput.Close()
        [void]$process.WaitForExit(30000)
    }
    $process.Dispose()
}
