from pathlib import Path
import subprocess, sys

r = Path(__file__).resolve().parents[2]
out = Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=True)
path = 'src/server/scripts/Legion/ReturnToKarazhan/karazhan_chess.cpp'
s = (subprocess.check_output(['git', '-C', str(r), 'show', 'be9d35ed166a2d07fd5360b2438e76a0c291b739:' + path]).decode('utf8')
     if len(sys.argv) > 2 and sys.argv[2] == 'baseline' else (r / path).read_text(encoding='utf8'))
a = s.index('    struct npc_karazhan_chessAI :')
b = s.index('\n    CreatureAI* GetAI', a)
(out / 'ChessAI.inc').write_bytes(s[a:b].encode('utf8'))
(out / 'ChessConstants.inc').write_bytes(s[s.index('#define SIZE_ONE_BOX'):s.index('// 115406')].encode('utf8'))
