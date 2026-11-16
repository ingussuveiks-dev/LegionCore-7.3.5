"""Known recent source inverses must preserve unknown edits and be idempotent."""
import artifact_recent_source_scope as scope
scope.verify()
count=0
for record in scope.PLANS:
 path=record['path'];source=(scope.ROOT/path).read_text(encoding='utf-8-sig')
 restored=scope.normalize(path,source)
 assert scope.sha(restored)==record['plans'][0]['before_sha256']
 assert scope.normalize(path,restored)==restored
 foreign=source+'\n// Unreviewed foreign source mutation\n'
 assert scope.normalize(path,foreign)==foreign and scope.sha(foreign)!=scope.sha(restored)
 for plan in record['plans']:
  for edit in plan['edits']:
   # A partially applied recent amendment cannot disappear into an old view.
   if edit['new'] not in source:continue
   partial=source.replace(edit['new'],edit['new']+'/* foreign */',1)
   assert scope.normalize(path,partial)==partial
   count+=1
print(f'PASS4 source files:exact inverse/replay,foreign full-file and{count} partial-edit controls;no active Warrior gameplay execution')
