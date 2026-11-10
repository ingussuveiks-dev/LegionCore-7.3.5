from pathlib import Path
import sys
from prepare_actor_queue import generate
r=Path(__file__).resolve().parents[2]
if len(sys.argv)>2 and sys.argv[2]=='HasabelWave':
 generate(r/'tools/tests/HasabelWaveFollowup.json','4524e27ed64cb3af59cccf00c2c9b7bcdc548197',sys.argv[1],sys.argv[2],len(sys.argv)>3 and sys.argv[3]=='baseline','HasabelWaveNative')
else:
 generate(r/'tools/tests/TwentyNinthTenRepairs.json','4524e27ed64cb3af59cccf00c2c9b7bcdc548197',sys.argv[1],sys.argv[2]if len(sys.argv)>2 else None,len(sys.argv)>3 and sys.argv[3]=='baseline','TwentyNinthNative')
 if len(sys.argv)==2:generate(r/'tools/tests/HasabelWaveFollowup.json','4524e27ed64cb3af59cccf00c2c9b7bcdc548197',sys.argv[1],None,False,'HasabelWaveNative')
