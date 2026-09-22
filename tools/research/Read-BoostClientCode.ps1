param([int]$ProcessId, [long]$Rva, [int]$Length=512)
$ErrorActionPreference='Stop'
$dll=Join-Path $env:TEMP 'boost-readonly-disassembly/iced/lib/net45/Iced.dll'
Add-Type -Path $dll
Add-Type -ReferencedAssemblies $dll -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Text;
using Iced.Intel;
public static class BoostCodeReader {
 [DllImport("kernel32.dll",SetLastError=true)] static extern IntPtr OpenProcess(uint access,bool inherit,int pid);
 [DllImport("kernel32.dll",SetLastError=true)] static extern bool ReadProcessMemory(IntPtr h,IntPtr addr,byte[] b,UIntPtr size,out UIntPtr read);
 [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
 class Output : FormatterOutput { public StringBuilder Text=new StringBuilder(); public override void Write(string text,FormatterTextKind kind){Text.Append(text);} }
 public static void Dump(int pid,long start,int count,long imageBase){
  var h=OpenProcess(0x410,false,pid);if(h==IntPtr.Zero)throw new Exception("Read-only OpenProcess failed");
  try {var b=new byte[count];UIntPtr read;if(!ReadProcessMemory(h,new IntPtr(start),b,(UIntPtr)count,out read)||read.ToUInt64()!=(ulong)count)throw new Exception("ReadProcessMemory failed");
   var decoder=Iced.Intel.Decoder.Create(64,new ByteArrayCodeReader(b));decoder.IP=(ulong)start;
   var formatter=new IntelFormatter();var output=new Output();
   var format=typeof(IntelFormatter).GetMethod("Format",new Type[]{typeof(Instruction).MakeByRefType(),typeof(FormatterOutput)});
   while(decoder.IP<(ulong)(start+count)){var ins=decoder.Decode();output.Text.Clear();format.Invoke(formatter,new object[]{ins,output});Console.WriteLine("RVA "+(ins.IP-(ulong)imageBase).ToString("X8")+"  "+output.Text);}
  } finally {CloseHandle(h);}
 }
}
'@
$process=Get-Process -Id $ProcessId
$base=$process.MainModule.BaseAddress.ToInt64()
[BoostCodeReader]::Dump($ProcessId,($base+$Rva),$Length,$base)
