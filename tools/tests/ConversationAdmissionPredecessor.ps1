function Get-PreConversationWorldAdmissionSource([string]$source) {
 if(!$source.Contains('// AddToMap owns the grid entry')){return $source}
 $path='src/server/game/Entities/Conversation/Conversation.cpp'
 $before=(& git -C $repo show "1bf86bb41:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned Conversation admission predecessor missing'}
 $factory=Block $source 'bool Conversation::CreateConversation('
 $guard=Block $factory 'if (!IsInWorld())'
 function AdmissionShape([string]$s){([regex]::Replace($s,'(?s)/\*.*?\*/','') -replace '//[^\n]*','') -replace '\s',''}
 if((AdmissionShape $guard) -cne (AdmissionShape 'if (!IsInWorld()) { GetMap()->RemoveFromMap(this, false); return false; }')){throw 'Only actual refused admission/caller-owned native cleanup is certified'}
 if((AdmissionShape $source.Replace($factory,$factory.Replace($guard,''))) -cne (AdmissionShape $before)){throw 'Other Conversation factory/lifetime/payload/DB loader body changed'}
 'PASS exact post-AddToMap world refusal/caller-owned cleanup successor; all other Conversation bodies retained'|Out-Host
 return $before
}
