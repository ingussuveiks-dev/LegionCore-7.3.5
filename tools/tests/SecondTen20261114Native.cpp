#include <unordered_set>
#include "Player.h"
#include "StringFormat.h"
#include "CommandFloatArgument.h"
#include <iostream>
#include <limits>
int main()
{
 auto positive=Trinity::StringFormat("%u",uint64(5000000000));
 auto negative=Trinity::StringFormat("%d",int64(-5000000000));
 std::cout<<"FORMAT\t"<<positive<<'\t'<<negative<<'\n';
 if(positive!="5000000000"||negative!="-5000000000")return 1;
 std::cout<<"NATIVE\t"<<MAX_MONEY_AMOUNT<<'\t'<<MAX_POWERS<<'\t'<<MOVE_RUN<<'\t'<<EquippedAvgItemLevel<<'\n';
 float original=7;
 if(Trinity::ParseCommandFloat("nan",original)||original!=7)return 2;
 if(!Trinity::ParseCommandFloat("0x1p1",original)||original!=2)return 3;
 std::cout<<"PASS actual linked formatter64-bit values and new float parser with native Player/Unit constants;no database/account/character/world mutation\n";
 return 0;
}
