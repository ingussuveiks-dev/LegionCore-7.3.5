# Only the independently tested final door may differ from the original class.
function Restore-DevourerDoorBoundary([string]$source){
 . "$PSScriptRoot/DevourerEscapeCertificate.ps1"
 $source=Restore-DevourerEscapeBoundary $source
 $old=(& git -C $repo show 'a8d3bb692:src/server/scripts/Scenario/Artifacts/Rogue/FangsoftheDevourer/Devourer.cpp') -join "`n"
 if($LASTEXITCODE){throw 'Pinned Wardstone door source missing'}
 $marker='class go_devourer_misc';$body=Block $source $marker;$prior=Block $old $marker
 if(($body-replace '\s','') -ceq ($prior-replace '\s','')){return $source}
 $normalized=$body.Replace('bool doorDone = false, doorBusy = false, doorActorWarning = false, doorCreditWarning = false;','')
 foreach($name in @('bool DoorLeaf(','Scenario* DoorContext(','bool DoorActor(','bool OpenDoor(','bool GossipUse(','uint32 GetData(')){$normalized=$normalized.Replace((Block $body $name),'')}
 $normalized=$normalized.Replace((Block $body 'bool GossipHello('),(Block $prior 'bool GossipHello('))
 if(($normalized-replace '\s','') -cne ($prior-replace '\s','')){throw 'Unreviewed original miscellaneous GO constructor/factory change'}
 return $source.Replace($body,$prior)
}
