param([switch]$VerifyLiveData,[switch]$VerifyNativeCatalog,[switch]$VerifyReleaseBinary,[switch]$VerifyAudio)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/fire-iceborn-story";$data="$repo/build-extractors/runtime-test/fire-iceborn-story-data"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($data,"$repo/build-extractors/runtime-test/script-pet-credit","$repo/build-extractors/runtime-test/legion-legendary-catalog") -Test {
 & "$PSScriptRoot/Test-FireIcebornStoryData.ps1" -VerifyLiveData:$VerifyLiveData
 & "$PSScriptRoot/Test-ScriptPetCredit.ps1" -InvokerCastRowsPath "$data/row.tsv" -BaselineInvokerCastRowsPath "$data/baseline-row.tsv" -AddressSanitizer -VerifyReleaseBinary:$VerifyReleaseBinary
 function Section([string]$source,[string]$start,[string]$end){$first=$source.IndexOf($start);$last=$source.IndexOf($end,[Math]::Max(0,$first));if($first -lt 0 -or $last -le $first){throw "Missing native section $start"};$source.Substring($first,$last-$first)}
 $source=Get-Content "$repo/src/server/game/Globals/ConversationData.h" -Raw
 [IO.File]::WriteAllText("$out/ConversationStructs.inc",(Section $source 'struct ConversationActor' 'typedef std::unordered_map'))
 $source=Get-Content "$repo/src/server/game/Entities/Conversation/Conversation.cpp" -Raw
 $begin=$source.IndexOf('bool Conversation::GenerateDynamicFieldData(');if($begin -lt 0){throw 'Missing native Conversation payload function'}
 [IO.File]::WriteAllText("$out/ConversationData.inc",$source.Substring($begin))
 [IO.File]::WriteAllText("$out/Lifetime.inc",(Section $source '    uint32 duration = 0;' '    if (!GetMap()->AddToMap(this))'))
 [IO.File]::WriteAllText("$out/PlayerVisibility.inc",(Section $source '    if (Player* player = caster->ToPlayer())' '    uint32 duration = 0;'))
 [IO.File]::WriteAllText("$out/Update.inc",(Section $source 'void Conversation::Update(uint32 p_time)' 'void Conversation::Remove()'))
 [IO.File]::WriteAllText("$out/Playback.inc",(Section $source 'bool Conversation::CanNeverSee2(' 'bool Conversation::LoadFromDB('))
 $source=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw
 [IO.File]::WriteAllText("$out/SummonConversation.inc",(Section $source 'void Spell::EffectSummonConversation(' 'void Spell::EffectLearnGarrisonBuilding('))
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/FireIcebornConversationTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif not %errorlevel%==0 exit /b %errorlevel%`n`"$out/test.exe`" `"$data/baseline-conversation.tsv`" `"$data/conversation.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Fire Iceborn conversation ownership/lifetime regression failed'}
 if($VerifyNativeCatalog){$old=$env:LEGION_DIALOGUE_CONFIG;try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile FireIcebornStoryCatalog.cpp -ProbeName FireIcebornStoryCatalog -OutputFile "$out/native.tsv"}finally{$env:LEGION_DIALOGUE_CONFIG=$old}
  $native=Get-Content "$out/native.tsv" -Raw
  if(!$native.Contains("EFFECT`t197760`t0`t0`t219`t0`t786`t0`t0`t18`t0`t0")){throw 'Native197760 ordinary219/786/caster destination differs'}
  if(!$native.Contains("CONVERSATION_LINE`t1870`t")){throw 'Native786 first line missing'}
  if(!$native.Contains("SCENARIO_EVENT`t47022`t957`t")){throw 'Native preserved Fire47022 criterion differs'}
 }
 if($VerifyAudio){if(!$VerifyNativeCatalog){throw 'Audio verification requires a fresh native catalog'};& "$PSScriptRoot/Test-FireIcebornAudio.ps1" -NativeCatalog "$out/native.tsv" -DataDirectory $data}
 if($VerifyReleaseBinary){$old=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_smart_player_cast_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Native opt-in caster Release path differs'}}finally{$env:PYTHONPATH=$old}}
}
