param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$plan=Get-Content "$PSScriptRoot/SeventhTenRepairs.json" -Raw|ConvertFrom-Json
$source='';$records=@()
foreach($case in $plan|Where-Object {$_.npc}){
 $edit=$case.edits[0].new;$line=($edit -split "`n"|Where-Object {$_ -match 'Player\s*\*\s*(?:player|pl)\s*=.*GetCharmerOrOwnerPlayerOrPlayerItself'}|Select-Object -First 1).Trim()
 if(!$line){throw 'Missing actual owner declaration'}
 $isIf=$line.StartsWith('if (');if($isIf){$line=$line.Substring(4).TrimEnd(')')+';'}
 $variable=if($line -match 'Player\s*\*\s*pl\s*='){'pl'}else{'player'}
 $source+="static Player* SeventhResolve$($case.key)(Unit*killer){$line return $variable;}`n"
 $records+="{$($case.npc),&SeventhResolve$($case.key)}"
}
$source+='struct SeventhResolver{uint32 entry;Player*(*resolve)(Unit*);};static SeventhResolver seventhResolvers[]={'+($records -join ',')+'};'
[IO.File]::WriteAllText("$OutputDirectory/SeventhResolvers.inc",$source)
