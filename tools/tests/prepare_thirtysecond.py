from pathlib import Path
import sys
from prepare_actor_queue import generate
r=Path(__file__).resolve().parents[2]
generate(r/'tools/tests/ThirtySecondTenRepairs.json','33e2d08e24a0cd91de74140af18c5272d087778e',sys.argv[1],sys.argv[2]if len(sys.argv)>2 else None,len(sys.argv)>3 and sys.argv[3]=='baseline','ThirtySecondNative')
