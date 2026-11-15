// Complete production FunctionProcessor.cpp and actual public header/STL.
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <string>
#include "FunctionProcessorActual.inc"
static void Require(bool ok,char const* message){if(!ok)throw std::runtime_error(message);}
static void Case(std::string const& name){
 FunctionProcessor p;uint32 old=0,fresh=0;
 if(name=="queued-rearm"||name=="active-rearm"){
  p.AddDelayedEvent(100,[&]{++old;});if(name=="active-rearm")p.Update(1);
  p.KillAllFunctions();p.AddDelayedEvent(100,[&]{++fresh;});p.Update(99);Require(!old&&!fresh,"cancel/rearm deadline changed");p.Update(1);Require(!old&&fresh==1,"new generation lost after cancellation");p.Update(60000);Require(!old&&fresh==1,"cancelled callback survived or new one repeated");
 }else if(name=="callback-cancel"){
  p.AddDelayedEvent(0,[&]{p.KillAllFunctions();});p.AddDelayedEvent(1,[&]{++old;});p.Update(10);Require(!old,"due callback ran after same-tick cancellation");
 }else if(name=="callback-rearm"){
  p.AddDelayedEvent(0,[&]{p.KillAllFunctions();p.AddDelayedEvent(5,[&]{++fresh;});});p.AddDelayedEvent(1,[&]{++old;});p.Update(10);Require(!old&&!fresh,"old due callback ran after cancel/rearm");p.Update(4);Require(!fresh,"rearmed callback ignored relative clock");p.Update(1);Require(fresh==1&&!old,"callback rearm lost new generation");
 }else if(name=="reentrant"){
  p.AddDelayedEvent(0,[&]{++fresh;Require(fresh==1,"executing callback replayed during recursive Update");p.Update(0);});p.Update(0);Require(fresh==1,"recursive callback not exactly once");
 }else if(name=="ordering"){
  std::vector<uint32> order;p.AddDelayedEvent(10,[&]{order.push_back(1);});p.AddDelayedEvent(10,[&]{order.push_back(2);});p.AddDelayedEvent(0,[&]{order.push_back(3);p.AddDelayedEvent(0,[&]{order.push_back(4);});});p.Update(0);Require(order==std::vector<uint32>{3},"newly queued callback ran in current tick");p.Update(0);Require(order==std::vector<uint32>({3,4}),"nested zero-delay callback lost");p.Update(9);Require(order.size()==2,"deadline moved");p.Update(1);Require(order==std::vector<uint32>({3,4,1,2}),"equal-deadline ordering changed");
 }else if(name=="empty-clock"){
  p.KillAllFunctions();p.Update(123);Require(p.CalculateTime(0)==123,"empty cancelled clock stopped");p.AddDelayedEvent(3,[&]{++fresh;});p.Update(2);Require(!fresh,"relative deadline shortened");p.Update(1);Require(fresh==1,"first new request after empty cancellation lost");
 }else if(name=="repeated-cancel"){
  p.AddDelayedEvent(0,[&]{++old;});p.KillAllFunctions();p.AddDelayedEvent(0,[&]{++old;});p.KillAllFunctions();p.AddDelayedEvent(0,[&]{++fresh;});p.Update(0);Require(!old&&fresh==1,"repeated cancel merged generations");
 }else throw std::runtime_error("Unknown case");
 std::cout<<"PASS actual FunctionProcessor "<<name<<"\n";
}
int main(int argc,char** argv){try{Require(argc==2,"case argument required");Case(argv[1]);return 0;}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
