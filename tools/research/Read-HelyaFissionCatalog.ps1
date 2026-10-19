param([string]$OutputFile='', [string]$DbcDirectory='')
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Read-LegionLegendaryCatalog.ps1" -SourceFile 'HelyaFissionCatalog.cpp' -ProbeName 'HelyaFissionCatalog' -OutputFile $OutputFile -DbcDirectory $DbcDirectory
