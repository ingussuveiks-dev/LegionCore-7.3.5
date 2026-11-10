from pathlib import Path
import sys
from prepare_actor_queue import generate
r=Path(__file__).resolve().parents[2]
generate(r/'tools/tests/ThirtyFirstTenRepairs.json','9cd171c1649b9d6428757db502748629afb0b583',sys.argv[1],sys.argv[2]if len(sys.argv)>2 else None,len(sys.argv)>3 and sys.argv[3]=='baseline','ThirtyFirstNative')
