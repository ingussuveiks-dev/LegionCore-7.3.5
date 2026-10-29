param([Parameter(Mandatory=$true)][string]$Executable,[Parameter(Mandatory=$true)][string[]]$Arguments,[Parameter(Mandatory=$true)][string]$InputFile,[Parameter(Mandatory=$true)][string]$OutputFile,[Parameter(Mandatory=$true)][string]$ErrorFile)
$ErrorActionPreference='Stop'
if([IO.Path]::GetFileName($Executable) -notin @('mysql.exe','mariadb.exe')){throw 'This helper owns only a native SQL client process'}
$inputPath=[IO.Path]::GetFullPath($InputFile);$outputPath=[IO.Path]::GetFullPath($OutputFile);$errorPath=[IO.Path]::GetFullPath($ErrorFile)
if(@($inputPath,$outputPath,$errorPath|Sort-Object -Unique).Count -ne 3){throw 'SQL input/stdout/stderr paths must be distinct'}
$start=[Diagnostics.ProcessStartInfo]::new();$start.FileName=$Executable;$start.UseShellExecute=$false;$start.CreateNoWindow=$true;$start.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
$start.RedirectStandardInput=$true;$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true;$start.StandardInputEncoding=[Text.UTF8Encoding]::new($false)
foreach($argument in $Arguments){$start.ArgumentList.Add($argument)}
$process=[Diagnostics.Process]::new();$process.StartInfo=$start
$outputStream=$null;$errorStream=$null;$started=$false
try{
 $sql=[IO.File]::ReadAllText($inputPath,[Text.Encoding]::UTF8)
 $outputStream=[IO.File]::Open($outputPath,[IO.FileMode]::Create,[IO.FileAccess]::Write,[IO.FileShare]::Read)
 $errorStream=[IO.File]::Open($errorPath,[IO.FileMode]::Create,[IO.FileAccess]::Write,[IO.FileShare]::Read)
 if(!$process.Start()){throw 'Native SQL client did not start'};$started=$true
 # Drain both pipes concurrently. A process exit/Start-Process -Wait alone did
 # not guarantee the final redirected bytes had arrived before disposal.
 $stdout=$process.StandardOutput.BaseStream.CopyToAsync($outputStream)
 $stderr=$process.StandardError.BaseStream.CopyToAsync($errorStream)
 [void]$process.StandardInput.WriteAsync($sql).GetAwaiter().GetResult()
 [void]$process.StandardInput.FlushAsync().GetAwaiter().GetResult();$process.StandardInput.Close()
 $process.WaitForExit()
 [void]$stdout.GetAwaiter().GetResult();[void]$stderr.GetAwaiter().GetResult()
 $outputStream.Flush();$errorStream.Flush()
 $process.ExitCode
}finally{
 if($started){try{$process.StandardInput.Close()}catch{};if(!$process.HasExited){$process.WaitForExit()}}
 if($outputStream){$outputStream.Dispose()};if($errorStream){$errorStream.Dispose()};$process.Dispose()
}
