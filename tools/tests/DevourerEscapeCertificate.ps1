# Only independently tested native cage/cache admission and cast guards differ.
function Restore-DevourerEscapeBoundary([string]$source){
 . "$PSScriptRoot/AkaariOpeningCertificate.ps1"
 $source=Restore-AkaariOpeningBoundary $source
 $old=(& git -C $repo show '47b792254:src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned native escape source missing'}
 if(!$source.Contains('namespace DevourerEscape')){return $source}
 $marker='class go_devourer_misc';$body=Block $source $marker;$prior=Block $old $marker
 foreach($name in @('bool GossipUse(','bool GossipHello(')){$source=$source.Replace((Block $body $name),(Block $prior $name))}
 $source=$source.Replace((Block $source 'namespace DevourerEscape'),'').Replace((Block $source 'class spell_devourer_escape')+';','')
 $source=$source.Replace('#include "SpellScript.h"','').Replace('new spell_devourer_escape();','')
 # Added comments are separate from the production callback certificate.
 $source=$source.Replace('// Native211689 cage release and211687 two-second weapons reclaim. CheckCast','').Replace('// runs again before a timed cast hits, before native scenario criteria dispatch.','')
 if(($source-replace '\s','') -cne ($old-replace '\s','')){throw 'Unreviewed source outside native cage/cache boundary'}
 return $source
}
