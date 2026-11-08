// Actual current header ABI with the matching Release constructor/library.
#include <vector>
#include <unordered_map>
#include <set>
#include <list>
#include <map>
#include <memory>
#include <functional>
#include "ObjectGuid.h"
#include "MovementPackets.h"
#include "SmartScript.h"
#include <iostream>
#include <cstdint>
template<class Tag,typename Tag::Type Member>struct LayoutAccess{friend typename Tag::Type Get(Tag){return Member;}};
#define FIELD(TagName,CppType,MemberName) struct TagName{using Type=CppType SmartScript::*;friend Type Get(TagName);};template struct LayoutAccess<TagName,&SmartScript::MemberName>;
FIELD(TimerTag,uint32,mTextTimer)
FIELD(TextTag,uint32,mLastTextID)
FIELD(GuidTag,ObjectGuid,mTextGUID)
FIELD(TalkerTag,uint32,mTalkerEntry)
FIELD(EnabledTag,bool,mUseTextTimer)
template<class Tag>void Print(SmartScript& object,char const* name,Tag tag){auto& member=object.*Get(tag);std::cout<<name<<'\t'<<(reinterpret_cast<std::uintptr_t>(&member)-reinterpret_cast<std::uintptr_t>(&object))<<'\t'<<sizeof(member)<<'\n';}
int main(){SmartScript object;std::cout<<"SIZE\t"<<sizeof(object)<<'\n';Print(object,"mTextTimer",TimerTag{});Print(object,"mLastTextID",TextTag{});Print(object,"mTextGUID",GuidTag{});Print(object,"mTalkerEntry",TalkerTag{});Print(object,"mUseTextTimer",EnabledTag{});return 0;}
