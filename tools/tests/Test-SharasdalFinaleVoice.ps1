param([switch]$VerifyReleaseBinary,[string]$DataDirectory='',[string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$DataDirectory){$DataDirectory="$repo/build-extractors/runtime-test/sharasdal-finale-voice-data"}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/sharasdal-finale-voice"}
$out=[IO.Path]::GetFullPath($OutputDirectory);New-Item -ItemType Directory -Force $out|Out-Null
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
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/SharasdalFinaleVoiceTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$DataDirectory/baseline-conversation.tsv`" `"$DataDirectory/conversation.tsv`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Sharasdal finale conversation lifetime regression failed'}
if($VerifyReleaseBinary){$previous=$env:PYTHONPATH;try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_hruthnir_conversation_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Actual native Conversation duration producer/expiry differs'}}finally{$env:PYTHONPATH=$previous}}
