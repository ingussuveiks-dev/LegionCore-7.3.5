"""Closed, strict UTF-8 reversals of Windows-1251/1252 misdecoding.

Only a unique exact single-pass byte roundtrip is accepted. ASCII script and
formatting tokens are invariant; correct Unicode, ambiguous/layered repairs,
replacement characters and new control characters are refused.
"""
UNDEFINED={'cp1252':{0x81,0x8d,0x8f,0x90,0x9d},'cp1251':{0x98}}
def single(text,codec):
 raw=bytearray()
 for char in text:
  try:raw.extend(char.encode(codec))
  except UnicodeEncodeError:
   if ord(char) not in UNDEFINED[codec]:raise
   raw.append(ord(char))
 fixed=bytes(raw).decode('utf-8','strict')
 original=''.join(chr(v) if v in UNDEFINED[codec] else bytes([v]).decode(codec) for v in raw)
 if original!=text or fixed==text or len(fixed.encode('utf-8'))>=len(text.encode('utf-8')):raise ValueError('Not an exact byte reduction')
 if any((ord(c)<32 and c not in '\r\n\t') or 127<=ord(c)<=159 or c=='�' for c in fixed):raise ValueError('Decoded control/replacement')
 if ''.join(c for c in text if ord(c)<128)!=''.join(c for c in fixed if ord(c)<128):raise ValueError('ASCII tokens changed')
 return fixed
def candidates(text):
 result=[]
 for codec in UNDEFINED:
  try:result.append((codec,single(text,codec)))
  except (ValueError,UnicodeError):pass
 return result
def recover(text):
 result=candidates(text)
 if len(result)!=1:raise ValueError('No unique single-pass recovery')
 codec,fixed=result[0]
 if candidates(fixed):raise ValueError('Layered encoding remains')
 return codec,fixed
