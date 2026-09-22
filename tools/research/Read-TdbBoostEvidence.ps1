param(
    [string]$DumpPath='C:/wamp64/www/TrinityCore/sql/TDB_full_1210.26091_2026_09_09/TDB_full_world_1210.26091_2026_09_09.sql',
    [string]$OutputPath
)
$ErrorActionPreference='Stop'
# Parse selected mysqldump VALUES tuples without executing any SQL.
if (-not ('BoostTdbScan' -as [type])) {
Add-Type -TypeDefinition @'
using System;
using System.IO;
using System.Collections.Generic;
using System.Text.RegularExpressions;
public class BoostTdbScan {
 public class Hit { public string Table; public int Line; public string Tuple; }
 public class Result {
  public List<Hit> Rows = new List<Hit>();
  public Dictionary<string,long> ScannedRows = new Dictionary<string,long>();
  public Dictionary<string,List<string>> Columns = new Dictionary<string,List<string>>();
 }
 public static Result Read(string path) {
  var ids=new HashSet<long>{105317,100448,114966,112565,107543,112038,112002,112026,112025,95093,105628,107953,107541,108988,108983,108982,101738,108478,100648,101824,103703,104530,107104,111995,111996,111998,112000,102592,109010,110747,112639,102658,111776,219615,219912,223763,227058,230503,231511,246606,246607,19768,20085,20459,29277,29837,30357};
  var maps=new HashSet<long>{1554,1557,1559,1560};
  var npcs=new HashSet<long>(); foreach(var id in ids) if(id>=95000 && id<=115000) npcs.Add(id);
  var gos=new HashSet<long>{246606,246607};
  var spells=new HashSet<long>{219615,219912,223763,227058,230503,231511};
  var menus=new HashSet<long>{19768,20085,20459};
  var texts=new HashSet<long>{29277,29837,30357};
  var tables=new HashSet<string>{"creature","gameobject","creature_template","creature_template_addon","creature_template_gossip","creature_text","smart_scripts","gameobject_template","transports","instance_template","vehicle_template_accessory","spell_script_names","spell_target_position","gossip_menu","gossip_menu_option","npc_text"};
  var tuple=new Regex(@"\((?:'(?:\\.|''|[^'\\])*'|[^'()])*\)",RegexOptions.Compiled);
  var prefix=new Regex(@"^\((-?\d+)(?:,(-?\d+))?(?:,(-?\d+))?",RegexOptions.Compiled);
  var insert=new Regex(@"^INSERT INTO `([^`]+)` VALUES ",RegexOptions.Compiled);
  var create=new Regex(@"^CREATE TABLE `([^`]+)`",RegexOptions.Compiled);
  var column=new Regex(@"^  `([^`]+)`",RegexOptions.Compiled);
  var result=new Result(); string schema=null; int lineNo=0;
  foreach(var line in File.ReadLines(path)) {
   lineNo++; var cm=create.Match(line);
   if(cm.Success){schema=cm.Groups[1].Value;if(tables.Contains(schema)) result.Columns[schema]=new List<string>();}
   else if(schema!=null && result.Columns.ContainsKey(schema)) {
    var fc=column.Match(line);if(fc.Success) result.Columns[schema].Add(fc.Groups[1].Value);
    if(line.StartsWith(")")) schema=null;
   }
   var im=insert.Match(line);if(!im.Success || !tables.Contains(im.Groups[1].Value)) continue;
   string table=im.Groups[1].Value;
   if(!result.ScannedRows.ContainsKey(table)) result.ScannedRows[table]=0;
   foreach(Match row in tuple.Matches(line,im.Length)) {
    result.ScannedRows[table]++;var pm=prefix.Match(row.Value);if(!pm.Success) throw new Exception("Unexpected numeric primary key in "+table);
    long first=long.Parse(pm.Groups[1].Value); bool keep=npcs.Contains(first);
    if(table=="creature" || table=="gameobject") keep=(table=="creature" ? npcs : gos).Contains(long.Parse(pm.Groups[2].Value)) || maps.Contains(long.Parse(pm.Groups[3].Value));
    else if(table=="instance_template") keep=maps.Contains(first);
    else if(table=="transports") keep=gos.Contains(long.Parse(pm.Groups[2].Value));
    else if(table=="gameobject_template") keep=gos.Contains(first);
    else if(table=="gossip_menu" || table=="gossip_menu_option") keep=menus.Contains(first);
    else if(table=="npc_text") keep=texts.Contains(first);
    else if(table=="spell_script_names" || table=="spell_target_position") keep=spells.Contains(first);
    else if(table=="smart_scripts") keep=keep || npcs.Contains(first/100) || gos.Contains(first);
    if(keep) result.Rows.Add(new Hit{Table=table,Line=lineNo,Tuple=row.Value});
   }
  }
  return result;
 }
}
'@
}
$scan=[BoostTdbScan]::Read((Resolve-Path $DumpPath).Path)
$result=[ordered]@{Source=(Split-Path $DumpPath -Leaf);SHA256=(Get-FileHash $DumpPath -Algorithm SHA256).Hash;Warning='Candidate records only. Different schema/build; never import this extract directly into legion_world.';ScannedRows=$scan.ScannedRows;Columns=$scan.Columns;Rows=$scan.Rows}
$json=$result | ConvertTo-Json -Depth 7
if($OutputPath){[IO.File]::WriteAllText($ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputPath),$json,[Text.UTF8Encoding]::new($false)); "Extracted $($scan.Rows.Count) candidate rows."}else{$json}
