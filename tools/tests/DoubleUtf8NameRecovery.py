"""Two uniquely reversible Windows-1252 layers; never partial/truncated repair.

Intermediate C1 bytes are allowed solely to reconstruct the next UTF-8 layer.
The final Cyrillic name must be printable and closed. Existing one-pass policy
in Utf8TextRecovery remains unchanged.
"""
from Utf8TextRecovery import UNDEFINED
def step(text,codec):
 raw=bytearray()
 for c in text:
  try:raw.extend(c.encode(codec))
  except UnicodeEncodeError:
   if ord(c) not in UNDEFINED[codec]:raise
   raw.append(ord(c))
 fixed=bytes(raw).decode('utf-8','strict')
 restored=''.join(chr(b) if b in UNDEFINED[codec] else bytes([b]).decode(codec) for b in raw)
 if restored!=text or fixed==text or len(fixed.encode())>=len(text.encode()):raise ValueError('Not a closed byte reduction')
 if ''.join(c for c in text if ord(c)<128)!=''.join(c for c in fixed if ord(c)<128):raise ValueError('ASCII changed')
 return fixed
def edges(text):
 out=[]
 for codec in UNDEFINED:
  try:out.append((codec,step(text,codec)))
  except (ValueError,UnicodeError):pass
 return out
def recover(text):
 first=edges(text)
 if len(first)!=1 or first[0][0]!='cp1252':raise ValueError('No unique first1252 layer')
 second=edges(first[0][1])
 if len(second)!=1 or second[0][0]!='cp1252':raise ValueError('No unique second1252 layer')
 fixed=second[0][1]
 if edges(fixed) or sum('\u0400'<=c<='\u04ff' for c in fixed)<2:raise ValueError('Not a closed Cyrillic name')
 if any(ord(c)<32 or 127<=ord(c)<=159 or c=='�' for c in fixed):raise ValueError('Final control/replacement')
 return fixed
