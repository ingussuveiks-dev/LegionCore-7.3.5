$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-VrektSceneRelease.ps1"
foreach($path in @('AI/CoreAI/CombatAI.cpp','AI/CoreAI/UnitAI.cpp','Spells/SpellInfo.cpp')){
 if((Get-Item "$repo/src/server/game/$path").LastWriteTimeUtc -gt (Get-Item $lib).LastWriteTimeUtc){throw "Actual final-link game library predates $path"}
}
$evidence="$repo/build-extractors/runtime-test/vrekt-combat-execution"
$hashes=Get-Content "$evidence/native-inputs.json" -Raw | ConvertFrom-Json
if($hashes.'canonical-worldserver.exe' -ne (Get-FileHash $exe -Algorithm SHA256).Hash -or
 $hashes.'actual-final-link-game.lib' -ne (Get-FileHash $lib -Algorithm SHA256).Hash){throw 'Run the native combat execution test against this Release first'}
$layout=[regex]::Match((Get-Content "$evidence/native-lines.txt" -Raw),'(?m)^LAYOUT (\d+) (\d+) (\d+) (\d+)\r?$')
if(!$layout.Success){throw 'Missing native SpellInfo layout evidence'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$dump=Get-ChildItem "$vs/VC/Tools/MSVC/*/bin/Hostx64/x64/dumpbin.exe" | Select-Object -Last 1
function Listing([string]$name,[string]$file){
 $range=Bounds $name
 $result=(& $dump.FullName /DISASM:NOBYTES ('/RANGE:0x{0:X},0x{1:X}' -f $range[0],$range[1]) $exe) -join "`n"
 if($LASTEXITCODE){throw 'Bounded native combat disassembly failed'}
 $result | Set-Content "$evidence/$file";$result
}
$admission=Listing '?CanAutoCast@SpellInfo@@' 'admission-disassembly.txt'
if($admission -notmatch '\bmov\s+rdi,rcx'){throw 'Re-audit compiled SpellInfo register binding'}
for($field=1;$field -le 4;$field++){
 $offset=([int]$layout.Groups[$field].Value).ToString('X')
 $width=if($field -le 2){'dword'}else{'byte'}
 if($admission -notmatch ($width+' ptr \[rdi\+0*'+$offset+'h\]')){throw 'Canonical admission lacks a native caster restriction field'}
}
$state=Resolve (Symbol '?HasAuraState@Unit@@')
if(@(@(Calls '?CanAutoCast@SpellInfo@@') | Where-Object {$_ -eq $state}).Count -lt 2){throw 'Canonical admission lacks both native caster-state checks'}
$dispatch=Listing '?UpdateAI@AggressorAI@@' 'aggressor-disassembly.txt'
# Recognize this MSVC build's policy0 or policies3..4 selection and the actual
# self pointer passed as the third CanAutoCast argument; old code passed victim.
$policy='test\s+eax,eax\s+[0-9A-F]+:\s+je\s+(?<self>[0-9A-F]+)\s+[0-9A-F]+:\s+add\s+eax,0FFFFFFFDh\s+[0-9A-F]+:\s+cmp\s+eax,1\s+[0-9A-F]+:\s+ja\s+(?<prepare>[0-9A-F]+)\s+\k<self>:\s+mov\s+r9,rdx\s+\k<prepare>:\s+mov\s+r8,r9'
if($dispatch -notmatch $policy){throw 'Re-audit deployed self/ally/buff preflight target dataflow'}
if(@(Calls '?UpdateAI@AggressorAI@@') -notcontains (Resolve (Symbol '?CanAutoCast@SpellInfo@@'))){throw 'Canonical Aggressor does not call native preflight'}
'PASS: canonical exception-bounded Aggressor implements self/ally/buff admission targeting; native-layout caster fields and both state calls are present. Same actual game library/executable hashes as the native behavior test.'
