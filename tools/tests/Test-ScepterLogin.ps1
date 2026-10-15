param([switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/scepter-login"
New-Item -ItemType Directory -Force -Path $output | Out-Null
function Export-Block($source,$first,$last,$file) {
    $a=$source.IndexOf($first)
    $b=$source.IndexOf($last,$a)
    if($a -lt 0 -or $b -le $a){throw "Missing production block: $first"}
    [IO.File]::WriteAllText("$output/$file",$source.Substring($a,$b-$a))
}
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
Export-Block $source '    // Restore the selector before CreateMap chooses' '    // NOW player must have valid map' 'EarlyRestore.inc'
if($source.IndexOf('    // Restore the selector before CreateMap chooses') -gt $source.IndexOf('    Map* map = sMapMgr->CreateMap(mapId, this);')){throw 'Selector restored too late'}
Export-Block $source '    if (!scenarioQuestStatusLoaded)' '    _LoadQuestStatusObjectives(' 'NormalQuestLoad.inc'
Export-Block $source '    // Keep a live Scepter instance' '    _LoadDailyQuestStatus(' 'ScepterRestore.inc'
$source=Get-Content "$repo/src/server/game/Maps/MapManager.cpp" -Raw
Export-Block $source '        uint32 introQuest =' '    InstanceTemplate const* instance' 'AccessWithBrace.inc'
$block=Get-Content "$output/AccessWithBrace.inc" -Raw
[IO.File]::WriteAllText("$output/Access.inc",$block.Substring(0,$block.LastIndexOf('    }')))
$source=Get-Content "$repo/src/server/game/Scenarios/Scenario.cpp" -Raw
Export-Block $source '    if (player->GetScenarioId())' '    _scenarioEntry =' 'ConstructorSelector.inc'
$source=Get-Content "$repo/src/server/game/Maps/Map.cpp" -Raw
Export-Block $source '        if (IsScenario() || sObjectMgr->HasScenarioInMap(GetId()))' '        // check for existing instance binds' 'AddScenario.inc'
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$output`" `"$PSScriptRoot/ScepterLoginTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$output/test.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Scepter login regression failed'}
if($VerifyReleaseBinary){
    $binary="$repo/build-extractors/bin/Release/worldserver.exe"
    $bytes=[IO.File]::ReadAllBytes($binary)
    $ascii=[Text.Encoding]::ASCII.GetString($bytes)
    if(!$ascii.Contains('An Eye for a Scepter: restoring player %u at the entrance in a fresh scenario instance')){
        throw 'Canonical Release executable does not contain the compiled Scepter login recovery; check the libraries used by the final link'
    }
    "PASS: canonical Release executable contains the production Scepter login recovery"
}
