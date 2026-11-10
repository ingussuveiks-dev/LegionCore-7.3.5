from pathlib import Path
import sys
from prepare_actor_queue import generate
r=Path(__file__).resolve().parents[2]
generate(r/'tools/tests/TwentySeventhTenRepairs.json','09e0bf8604de220de85ddc32e9339ccf3616c8d3',sys.argv[1],sys.argv[2]if len(sys.argv)>2 else None,len(sys.argv)>3 and sys.argv[3]=='baseline','TwentySeventhNative')
