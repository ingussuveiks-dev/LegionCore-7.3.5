"""Resolve the known local CreatureAI overload family before projecting a queue.
Unknown inheritance/queue owners require inspection instead of a Unit assumption.
"""
import re

def queue_owner(entry,source,callback):
 masked=re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',lambda m:re.sub(r'[^\n]',' ',m[0]),source,flags=re.S)
 declarations={m[1]:m for m in re.finditer(r'(?:struct|class)\s+(\w+)\s*:\s*([^;{]+)\{',masked)}
 parents={name:re.sub(r'\b(public|protected|private|virtual)\b','',m[2]).strip()for name,m in declarations.items()}
 core={'ScriptedAI':'CreatureAI','Scripted_NoMovementAI':'ScriptedAI','BossAI':'ScriptedAI','WorldBossAI':'ScriptedAI','BrawlersBossAI':'ScriptedAI','NullCreatureAI':'CreatureAI','SmartAI':'CreatureAI','CosmeticAI':'CreatureAI'}
 lineage=[];current=entry['cls']
 while current!='CreatureAI':
  if current in lineage:raise ValueError('Inheritance cycle in queue ownership')
  if callback['delegated']and current not in core and current in declarations:
   start=declarations[current].end()-1;end=start+1;depth=1
   while depth and end<len(masked):depth+=(masked[end]=='{')-(masked[end]=='}');end+=1
   body=masked[start:end];method='AddDelayed'+('Combat'if callback['queue']=='combat'else'Event')
   for declared in re.finditer(r'\b(?:void|auto|[A-Za-z_]\w*(?:::\w+)*(?:<[^;{}]*>)?)\s+'+method+r'\s*\(',body):
    if body[:declared.start()].count('{')-body[:declared.start()].count('}')==1:raise ValueError('Unproven local queue overload: '+current+'::'+method)
  lineage.append(current);parent=core.get(current,parents.get(current))
  if not parent or ','in parent or '<'in parent:raise ValueError('Unproven queue inheritance: '+current)
  current=parent.split('::')[-1].strip()
 if callback['queue_owner']!='me':raise ValueError('Unproven delayed queue object: '+callback['queue_owner'])
 if not callback['delegated']:return 'Unit'
 if callback['queue']=='event'and 'CosmeticAI'in lineage:return 'AI'
 return 'Unit'
