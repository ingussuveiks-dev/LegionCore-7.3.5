#include <cassert>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include <iostream>
using uint8=uint8_t;using uint32=uint32_t;using uint64=uint64_t;
#define UI64LIT(x) x##ULL
#define TC_LOG_DEBUG(...) ((void)0)
#include "Types.inc"
struct Field{uint64 v=0;uint8 GetUInt8(){return uint8(v);}uint32 GetUInt32(){return uint32(v);}uint64 GetUInt64(){return v;}};
struct Result{std::vector<std::vector<Field>> rows;size_t pos=0;Field* Fetch(){return rows[pos].data();}bool NextRow(){return ++pos<rows.size();}};
using PreparedQueryResult=std::shared_ptr<Result>;
enum{CHAR_INS_CHAR_ACTION,CHAR_UPD_CHAR_ACTION,CHAR_DEL_CHAR_ACTION_BY_BUTTON_SPEC};
struct CharacterDatabasePreparedStatement{int kind;uint64 args[5]{};void setUInt64(int i,uint64 v){args[i]=v;}void setUInt8(int i,uint8 v){args[i]=v;}};
struct Transaction{std::vector<std::unique_ptr<CharacterDatabasePreparedStatement>> statements;void Append(CharacterDatabasePreparedStatement* p){statements.emplace_back(p);}};
using CharacterDatabaseTransaction=std::shared_ptr<Transaction>;
using Key=std::tuple<uint64,uint8,uint8>;using Value=std::pair<uint64,uint8>;
struct Database{
 std::map<Key,Value> rows;
 CharacterDatabasePreparedStatement* GetPreparedStatement(int kind){auto p=new CharacterDatabasePreparedStatement;p->kind=kind;return p;}
 void Commit(CharacterDatabaseTransaction t){for(auto const& p:t->statements){auto a=p->args;
  if(p->kind==CHAR_INS_CHAR_ACTION)rows[{a[0],uint8(a[1]),uint8(a[2])}]={a[3],uint8(a[4])};
  if(p->kind==CHAR_UPD_CHAR_ACTION){auto i=rows.find({a[2],uint8(a[4]),uint8(a[3])});if(i!=rows.end())i->second={a[0],uint8(a[1])};}
  if(p->kind==CHAR_DEL_CHAR_ACTION_BY_BUTTON_SPEC)rows.erase({a[0],uint8(a[2]),uint8(a[1])});
 }}
 PreparedQueryResult Load(uint8 spec){auto r=std::make_shared<Result>();for(auto const& p:rows)if(std::get<0>(p.first)==1&&std::get<1>(p.first)==spec)r->rows.push_back({{std::get<2>(p.first)},{p.second.first},{p.second.second}});return r->rows.empty()?nullptr:r;}
}CharacterDatabase;
struct Guid{std::string ToString(){return "test";}};
struct Player{
 ActionButtonList m_actionButtons;std::set<uint64> known;uint8 spec=0;
 char const* GetName(){return "test";}Guid GetGUID(){return {};}uint64 GetGUIDLow(){return 1;}uint8 GetActiveTalentGroup(){return spec;}
 bool IsActionButtonDataValid(uint8 button,uint64 action,uint8 type){return button<MAX_ACTION_BUTTONS&&action<MAX_ACTION_BUTTON_ACTION_VALUE&&(type!=ACTION_BUTTON_SPELL||known.count(action));}
 ActionButton* AddActionButton(uint8,uint64,uint8);void RemoveActionButton(uint8);ActionButton const* GetActionButton(uint8);
 void _LoadActions(PreparedQueryResult,bool preserveLearnedActions=false);void _SaveActions(CharacterDatabaseTransaction&);
 // Spell classification is outside this persistence harness. Keep the real
 // helper's duplicate/empty-slot contract, with all fixture spells active.
 bool AddSpellToActionBarIfAppropriate(uint32 spell,bool){if(!known.count(spell))return false;for(auto const& a:m_actionButtons)if(a.second.uState!=ACTIONBUTTON_DELETED&&a.second.GetType()==ACTION_BUTTON_SPELL&&a.second.GetAction()==spell)return false;
  for(uint8 i=0;i<72;++i)if(!GetActionButton(i))return AddActionButton(i,spell,ACTION_BUTTON_SPELL)!=nullptr;return false;}
 void Save(){auto t=std::make_shared<Transaction>();_SaveActions(t);CharacterDatabase.Commit(t);}
};
#include "Add.inc"
#include "Remove.inc"
#include "Load.inc"
#include "Save.inc"
bool Has(Player const& p,uint64 spell){for(auto const& a:p.m_actionButtons)if(a.second.uState!=ACTIONBUTTON_DELETED&&a.second.GetAction()==spell)return true;return false;}
int main(){
 constexpr uint64 macro=0x100000001ULL;
 Player p;p.known={100,200,300};p.AddActionButton(0,100,0);p.AddActionButton(2,999,0);p.AddActionButton(8,macro,ACTION_BUTTON_MACRO);p.Save();auto original=CharacterDatabase.rows;
 // First switch to an empty specialization, with an unchanged shared spell,
 // a formerly valid old-spec spell and a newly auto-placed spell.
 p.AddActionButton(2,200,0);p.Save();original=CharacterDatabase.rows;p.known.erase(200);p.RemoveActionButton(2);p.AddActionButton(2,300,0);assert(p.GetActionButton(2)->uState==ACTIONBUTTON_CHANGED);
 p.spec=1;p._LoadActions(CharacterDatabase.Load(1),true);
 assert(Has(p,100)&&Has(p,300)&&!Has(p,200)&&p.GetActionButton(8)->GetAction()==macro);
 for(auto const& a:p.m_actionButtons)assert(a.second.uState==ACTIONBUTTON_NEW);
 p.Save();assert(CharacterDatabase.rows.at({1,1,2}).first==300);
 for(auto const& row:original)assert(CharacterDatabase.rows.at(row.first)==row.second);
 Player relog;relog.spec=1;relog.known=p.known;relog._LoadActions(CharacterDatabase.Load(1));assert(Has(relog,100)&&Has(relog,300)&&relog.GetActionButton(8)->GetAction()==macro);
 // Existing destination: retain custom positions/macro/item, merge auto-added
 // NEW and CHANGED spells, ignore old-spec UNCHANGED and DELETED buttons.
 CharacterDatabase.rows[{1,2,20}]={100,0};CharacterDatabase.rows[{1,2,0}]={macro,ACTION_BUTTON_MACRO};CharacterDatabase.rows[{1,2,1}]={12345,ACTION_BUTTON_ITEM};
 p.AddActionButton(4,400,0); // unknown spell rejected
 p.known.insert(400);p.AddActionButton(4,400,0);p.known.insert(500);p.AddActionButton(2,500,0);p.RemoveActionButton(8);
 p.spec=2;p._LoadActions(CharacterDatabase.Load(2),true);
 assert(p.GetActionButton(0)->GetAction()==macro&&p.GetActionButton(1)->GetAction()==12345&&p.GetActionButton(20)->GetAction()==100);
 assert(Has(p,400)&&Has(p,500)&&!Has(p,300));p.Save();auto saved=CharacterDatabase.rows;
 for(int i=0;i<5;++i){p._LoadActions(CharacterDatabase.Load(2),true);p.Save();assert(CharacterDatabase.rows==saved);}
 p.RemoveActionButton(20);p.Save();assert(!CharacterDatabase.rows.count({1,2,20}));
 Player finalLogin;finalLogin.spec=2;finalLogin.known=p.known;finalLogin._LoadActions(CharacterDatabase.Load(2));assert(Has(finalLogin,400)&&Has(finalLogin,500)&&!Has(finalLogin,100));
 // An invalid saved spell is still deleted normally; empty login never merges
 // old in-memory state. Manual same-spec talent replacement persists as UPDATE.
 CharacterDatabase.rows[{1,2,30}]={999999,0};finalLogin._LoadActions(CharacterDatabase.Load(2));assert(!finalLogin.GetActionButton(30));finalLogin.Save();assert(!CharacterDatabase.rows.count({1,2,30}));
 finalLogin.AddActionButton(5,400,0);finalLogin.Save();finalLogin.AddActionButton(5,500,0);finalLogin.Save();assert(CharacterDatabase.rows.at({1,2,5}).first==500);
 finalLogin._LoadActions(nullptr);assert(finalLogin.m_actionButtons.empty());
 std::cout<<"PASS: native action persistence lifecycle and specialization isolation\n";
}
