from pathlib import Path
import sys
from prepare_lifetime_queue import generate
r=Path(__file__).resolve().parents[2]
generate(r/'tools/tests/FortyFourthTenRepairs.json','c3912fe96074cff13a49853523c1092d677a2d63',sys.argv[1],sys.argv[2]if len(sys.argv)>2 else None,len(sys.argv)>3 and sys.argv[3]=='baseline','FortyFourthNative')
