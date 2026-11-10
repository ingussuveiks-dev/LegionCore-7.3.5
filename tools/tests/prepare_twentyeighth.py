from pathlib import Path
import sys
from prepare_actor_queue import generate
r=Path(__file__).resolve().parents[2]
generate(r/'tools/tests/TwentyEighthTenRepairs.json','828ecbbf58ba7f5ce66ad9e46152bfc2afae8ca7',sys.argv[1],sys.argv[2]if len(sys.argv)>2 else None,len(sys.argv)>3 and sys.argv[3]=='baseline','TwentyEighthNative')
