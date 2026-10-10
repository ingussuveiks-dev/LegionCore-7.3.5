param([string]$OutputDirectory = "$PSScriptRoot/../../build-extractors/runtime-test/eye-quest-chain")
$ErrorActionPreference = 'Stop'
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
foreach ($header in @('ScriptMgr.h','ScriptedCreature.h','Player.h','ObjectAccessor.h','Map.h','TemporarySummon.h','GameObject.h','DatabaseEnv.h','ScriptedGossip.h','QuestData.h','AreaTriggerAI.h','SpellMgr.h','Vehicle.h','QuestDef.h','GameObjectAI.h')) {
    [IO.File]::WriteAllText("$output/$header", '#pragma once')
}
$source=Get-Content "$PSScriptRoot/../../src/server/scripts/Legion/EyeofAzshara/instance_eye_of_azshara.cpp" -Raw
$start=$source.IndexOf('        ObjectGuid SerpentrixGUID;')
$end=$source.IndexOf('        void Update(uint32 diff) override',$start)
if($start -lt 0 -or $end -le $start){throw 'Cannot extract production instance methods'}
[IO.File]::WriteAllText("$output/EyeInstanceMethods.inc", "struct EyeInstance:InstanceScript { EyeInstance(InstanceMap* m):InstanceScript(m){}`n"+$source.Substring($start,$end-$start)+"};`n")
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual C++ compiler not found' }
$commands = @"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /W4 /I"$output" "$PSScriptRoot/EyeQuestChainTest.cpp" /Fe"$output/EyeQuestChainTest.exe" /Fo"$output/EyeQuestChainTest.obj"
if errorlevel 1 exit /b 1
"$output/EyeQuestChainTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd", $commands)
& "$output/run.cmd"
if ($LASTEXITCODE) { throw 'Eye quest chain regression failed' }
