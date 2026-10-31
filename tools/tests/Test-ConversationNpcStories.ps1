param([string]$OutputDirectory='', [string]$DataRoot='', [string[]]$Cases=@())
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-ConversationNpcLifetime.ps1" -ScopeOnly
if(!$?){throw 'Current native NPC lifetime source certificate failed'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/conversation-npc-stories"};$out=[IO.Path]::GetFullPath($OutputDirectory)
if(!$DataRoot){$DataRoot="$repo/build-extractors/runtime-test"};$data=[IO.Path]::GetFullPath($DataRoot)
& "$PSScriptRoot/Test-SharasdalFinaleVoice.ps1" -PrepareHost -OutputDirectory "$out/base"
if(!$?){throw 'Current native generation/creation/viewer/update host preparation failed'}
$voice=Get-Content "$PSScriptRoot/SharasdalFinaleVoiceTest.cpp" -Raw;$end=$voice.IndexOf('static void Load(');if($end -lt 0){throw 'Shared complete voice host boundary missing'};[IO.File]::WriteAllText("$out/base/VoiceHost.inc",$voice.Substring(0,$end))
$s=Get-Content "$repo/src/server/game/Spells/SpellEffects.cpp" -Raw;$a=$s.IndexOf('void Spell::EffectSummonConversation(');$b=$s.IndexOf('void Spell::EffectLearnGarrisonBuilding(',$a);if($a -lt 0 -or $b -lt $a){throw 'Native effect219 boundary missing'};[IO.File]::WriteAllText("$out/base/SummonConversation.inc",$s.Substring($a,$b-$a))
$matrix=@(
 @('farewell','ConversationFarewellTest.cpp',@('marksmanship-farewell/conversation.tsv')),
 @('interrogation','ConversationInterrogationTest.cpp',@('marksmanship-interrogation-budget/conversations.tsv')),
 @('hruthnir','HruthnirConversationTest.cpp',@('hruthnir-conversation/conversation.tsv')),
 @('magnar','MagnarConversationsTest.cpp',@('magnar-conversations/conversation.tsv')),
 @('magnar-timing','MagnarDialogueTimingTest.cpp',@('magnar-dialogue-timing/conversation.tsv','magnar-dialogue-timing/voices.tsv')),
 @('titanstrike-finale','TitanstrikeFinaleConversationTest.cpp',@('titanstrike-finale-conversation/conversation.tsv')),
 @('titanstrike-aftermath','TitanstrikeAftermathTest.cpp',@('titanstrike-aftermath/conversation.tsv')),
 @('fire','FireIcebornConversationTest.cpp',@('fire-iceborn-story-data/baseline-conversation.tsv','fire-iceborn-story-data/conversation.tsv')),
 @('sothozz','SothozzConversationTest.cpp',@('sothozz-invoker-data/conversation.tsv')),
 @('sharasdal-finale','SharasdalFinaleVoiceTest.cpp',@('sharasdal-finale-voice-data/baseline-conversation.tsv','sharasdal-finale-voice-data/conversation.tsv')),
 @('sharasdal-upper','SharasdalUpperVoiceTest.cpp',@('sharasdal-upper-voice-data/baseline-conversation.tsv','sharasdal-upper-voice-data/conversation.tsv'))
)
foreach($name in $Cases){if($name -notin @($matrix|ForEach-Object {$_[0]})){throw "Unknown retained story: $name"}}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$manifest=@()
foreach($case in $matrix){
 $name=$case[0];if($Cases.Count -and $name -notin $Cases){continue};$caseOut="$out/$name";New-Item -ItemType Directory -Force $caseOut|Out-Null
 Get-ChildItem "$out/base" -Filter '*.inc' -File|ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination $caseOut}
 $arguments=@();$inputs=@();$index=0
 foreach($relative in $case[2]){$inputPath=Join-Path $data $relative;if(!(Test-Path -LiteralPath $inputPath)){throw "Missing retained native definition; run its existing data suite first: $relative"};$before=(Get-FileHash -LiteralPath $inputPath).Hash;$copy="$caseOut/input-$index.tsv";Copy-Item -LiteralPath $inputPath -Destination $copy;if((Get-FileHash -LiteralPath $copy).Hash -ne $before -or (Get-FileHash -LiteralPath $inputPath).Hash -ne $before){throw 'Retained input changed while copying'};$arguments+='"'+$copy+'"';$inputs+=@{Source=$relative;SHA256=$before};++$index}
 [IO.File]::WriteAllText("$caseOut/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$caseOut`" `"$PSScriptRoot/$($case[1])`" /Fe`"$caseOut/test.exe`" /Fo`"$caseOut/test.obj`" /Fd`"$caseOut/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$caseOut`"`n`"$caseOut/test.exe`" $($arguments -join ' ')`nexit /b %errorlevel%`n")
 & "$caseOut/run.cmd";if($LASTEXITCODE){throw "Native retained story failed: $name"}
 $manifest+=@{Case=$name;Source=$case[1];SourceSHA256=(Get-FileHash "$PSScriptRoot/$($case[1])").Hash;Inputs=$inputs;Result='PASS current production payload/budget/visibility/update under ASAN; retained prepared definitions, no fresh live data/client claim'}
 [IO.File]::WriteAllText("$out/manifest.json",($manifest|ConvertTo-Json -Depth 6))
}
"PASS $($manifest.Count) retained story suites compile current production Conversation bodies with hash-preserved prepared definitions; no live SQL mutation or client playback claim"
