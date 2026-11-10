from pathlib import Path
import sys
from prepare_actor_queue import generate
r=Path(__file__).resolve().parents[2]
generate(r/'tools/tests/ThirtiethTenRepairs.json','41e54cb4e8cd2b243d51a7709e83c592a2230514',sys.argv[1],sys.argv[2]if len(sys.argv)>2 else None,len(sys.argv)>3 and sys.argv[3]=='baseline','ThirtiethNative')
