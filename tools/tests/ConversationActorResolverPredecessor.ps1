function Get-PreConversationActorResolverSource([string]$source) {
 if(!$source.Contains('Creature* Conversation::FindCreatureActor(')){return $source}
 $expected=@'
Creature* Conversation::FindCreatureActor(uint32 /*index*/, uint32 entry, WorldObject* searcher)
{
    return searcher->FindNearestCreature(entry, searcher->GetVisibilityRange());
}
'@
 $body=Block $source 'Creature* Conversation::FindCreatureActor('
 if(($body -replace '\s','') -cne ($expected -replace '\s','')){throw 'Ordinary Conversation actor resolver changed'}
 $call='FindCreatureActor(itr.id, itr.creatureId, sercher)'
 if(([regex]::Matches($source,[regex]::Escape($call))).Count -ne 1){throw 'Unexpected scoped resolver call sites'}
 return $source.Replace($body,'').Replace($call,'sercher->FindNearestCreature(itr.creatureId, sercher->GetVisibilityRange())')
}
function Get-PreConversationActorResolverHeader([string]$source) {
 $addition=@'
        // Scripts may bind an authored creature slot to an exact source actor.
        // The ordinary resolver deliberately continues to require a living NPC.
        virtual Creature* FindCreatureActor(uint32 index, uint32 entry, WorldObject* searcher);
'@
 $source=$source.Replace("`r",'');$addition=$addition.Replace("`r",'')
 if(!$source.Contains($addition)){throw 'Expected isolated appended Conversation virtual resolver missing'}
 return $source.Replace($addition,'')
}
