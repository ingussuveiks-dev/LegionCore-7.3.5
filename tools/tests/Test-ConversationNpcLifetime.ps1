param([switch]$ScopeOnly,[switch]$Baseline,[switch]$PrepareHost,[switch]$VerifyReleaseBinary,[string]$OutputDirectory='')
. "$PSScriptRoot/ConversationAdmissionPredecessor.ps1"
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
function Block([string]$s,[string]$start){$a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth -and $b -lt $s.Length){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};if($depth){throw "Unclosed $start"};$s.Substring($a,$b-$a)}
$path='src/server/game/Entities/Conversation/Conversation.cpp';$header='src/server/game/Entities/Conversation/Conversation.h'
$source=(Get-Content "$repo/$path" -Raw).Replace("`r",'');$source=Get-PreConversationWorldAdmissionSource $source;$old=(& git -C $repo show "9a38534f0:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned NPC conversation source missing'}
$oldHeader=(& git -C $repo show "9a38534f0:$header") -join "`n";if($LASTEXITCODE){throw 'Pinned NPC conversation header missing'}
$members=(Get-PreConversationActorResolverHeader (Get-Content "$repo/$header" -Raw)).Replace("`r",'').Replace('        uint32 _createdAt = 0;','').Replace('        bool _npcCaster = false;','')
if(($members -replace '\s','') -cne ($oldHeader -replace '\s','')){throw 'Other Conversation API/vtable/earlier fields changed'}
$normal=$source.Replace('    _createdAt = getMSTime();','').Replace('    _npcCaster = !caster->IsPlayer();','')
$update=Block $normal 'void Conversation::Update('
$branch=@'
        uint32 const deadline = GetUInt32Value(CONVERSATION_FIELD_LAST_LINE_END_TIME);
        if (_npcCaster)
        {
            uint32 const now = getMSTime();
            if (now - _createdAt <= deadline)
                return;
            // NPCs never acquire a Player playback key. Close their native
            // admission window, but let every already admitted viewer finish
            // their own full published clip before removing this one-shot.
            for (auto const& viewer : playing)
                if (now - viewer.second <= deadline)
                    return;
            Remove();
            return;
        }
'@
if(!$update.Contains($branch.Replace("`r",''))){throw 'NPC native window/full existing viewer cleanup changed'}
$restored=$update.Replace($branch.Replace("`r",''),'').Replace('if (dur > deadline)','if (dur > GetUInt32Value(CONVERSATION_FIELD_LAST_LINE_END_TIME))')
$normal=$normal.Replace($update,$restored)
$view=Block $normal 'bool Conversation::CanNeverSee2('
$gate=@'
        if (_npcCaster && GetDuration() && now - _createdAt > GetUInt32Value(CONVERSATION_FIELD_LAST_LINE_END_TIME))
            return true;
'@
if(!$view.Contains($gate.Replace("`r",''))){throw 'NPC new-viewer admission window changed'}
$restored=$view.Replace($gate.Replace("`r",''),'').Replace('    uint32 const now = getMSTime();','').Replace(' = now;',' = getMSTime();').Replace('uint32 const dur = now - data->second;','uint32 const dur = getMSTime() - data->second;')
$normal=$normal.Replace($view,$restored)
if(($normal -replace '\s','') -cne ($old -replace '\s','')){throw 'Other generation/budget/payload/player/group/quest/static/factory/removal behavior changed'}
if($ScopeOnly){'PASS only dynamic NPC creation timestamp/type, native budget admission and existing-viewer full deadline cleanup; player/static/quest scope/payload/budgets/registration/earlier fields retained';return}
if($Baseline -and $VerifyReleaseBinary){throw 'Counterfactual cannot certify Release'}
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/conversation-npc-lifetime$(if($Baseline){'-baseline'})"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Test-SharasdalFinaleVoice.ps1" -PrepareHost -OutputDirectory $out
if(!$?){throw 'Actual native conversation host preparation failed'}
if($Baseline){[IO.File]::WriteAllText("$out/Update.inc",(Block $old 'void Conversation::Update('));[IO.File]::WriteAllText("$out/Playback.inc",(Block $old 'bool Conversation::CanNeverSee2('));$a=$old.IndexOf('    uint32 duration = 0;');$b=$old.IndexOf('    if (!GetMap()->AddToMap(this))',$a);[IO.File]::WriteAllText("$out/Lifetime.inc",$old.Substring($a,$b-$a))}
$hostSource=Get-Content "$PSScriptRoot/SharasdalFinaleVoiceTest.cpp" -Raw;$end=$hostSource.IndexOf('static void Load(');if($end -lt 0){throw 'Native shared voice host boundary changed'};[IO.File]::WriteAllText("$out/Host.inc",$hostSource.Substring(0,$end))
if($PrepareHost){return}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath;$mode=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $mode /I`"$out`" `"$PSScriptRoot/ConversationNpcLifetimeTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native NPC conversation window/lifetime regression failed'}
if($VerifyReleaseBinary){
 $python='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& $python "$PSScriptRoot/verify_conversation_npc_lifetime_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Release NPC lifetime verification failed'}}finally{$env:PYTHONPATH=$previous}
}
