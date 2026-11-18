#define HARBARON_NATIVE_SUPPORT_ONLY
#include "HarbaronFragmentNative.cpp"
#ifdef DYNAMIC_MODEL_NEAREST_BASELINE
#include "DynamicTreeActual.inc"
#endif

int main(int argc,char**argv)
{
    try
    {
        InitializeDungeon(argc,argv);MapInstanced parent(1492,300);InstanceMap map(1492,300,9948400,DIFFICULTY_MYTHIC_KEYSTONE,&parent);map.CreateInstanceData(nullptr);
        unsigned different=0,matched=0;
        {
            DungeonShip ship(map);DynamicMapTree tree;tree.insert(*ship.m_model);tree.balance();
            for(int ix=-4;ix<=4;++ix)for(int iy=-4;iy<=4;++iy)
            {
                float x=2906.87f+ix*0.25f,y=831.485f+iy*0.25f,z=515.16f;
                float direct=ship.m_model->getHeight(x,y,z,3.f,ship.GetPhases(),false);
                DynamicTreeCallback receipt;float actual=tree.getHeight(x,y,z,3.f,ship.GetPhases(),false,&receipt);
                Check(std::isfinite(direct)&&std::isfinite(actual)&&receipt.go==&ship,"Actual shared model height/support receipt absent");
                bool nearest=std::fabs(direct-actual)<0.001f;
                different+=!nearest;matched+=nearest;
#ifndef DYNAMIC_MODEL_NEAREST_BASELINE
                Check(nearest,"Dynamic tree chose a farther triangle within the same model");
#endif
                G3D::Vector3 from(x,y,z+0.5f),to(x,y,z-2.5f),hit;
                DynamicTreeCallback contact;
                Check(tree.getObjectHitPos(ship.GetPhases(),false,from,to,hit,0.f,&contact)&&contact.go==&ship,"Actual shared model contact receipt absent");
#ifndef DYNAMIC_MODEL_NEAREST_BASELINE
                Check(std::fabs(hit.z-direct)<0.001f,"Contact query did not stop at closest real surface");
#endif
                Check(!tree.isInLineOfSight(from,to,ship.GetPhases(),false),"Any-hit LOS stopped detecting the real floor");
            }
#ifdef DYNAMIC_MODEL_NEAREST_BASELINE
            Check(different>0,"Pinned first-hit defect was not reproduced");
#else
            Check(different==0&&matched==81,"Nearest-surface sample coverage changed");
#endif
            DynamicTreeCallback filtered;filtered.filter=[](GameObject const*){return false;};
            Check(!std::isfinite(tree.getHeight(2906.87f,831.485f,515.16f,3.f,ship.GetPhases(),false,&filtered))&&!filtered.go,"Caller collision filter was bypassed");
            Check(!std::isfinite(tree.getHeight(2906.87f,831.485f,520.f,0.1f,ship.GetPhases(),false)),"Height query exceeded native search distance");
            ship.EnableCollision(false);Check(!std::isfinite(tree.getHeight(2906.87f,831.485f,515.16f,3.f,ship.GetPhases(),false)),"Disabled model collision still contributed ground");ship.EnableCollision(true);
            tree.remove(*ship.m_model);
            GameObject lower;Position pose{2933.33f,800.f,499.75f,3.14159f};
            Check(lower.Create(189800,246919,&map,1,pose,G3D::Quat(2.5924e-43f,0,0,1),255,GO_STATE_READY),"Controlled overlapping native model create failed");lower.EnableCollision(true);
            for(unsigned order=0;order<2;++order)
            {
                if(order){tree.insert(*ship.m_model);tree.insert(*lower.m_model);}else{tree.insert(*lower.m_model);tree.insert(*ship.m_model);}tree.balance();
                for(int ix=-4;ix<=4;++ix)for(int iy=-4;iy<=4;++iy)
                {
                    float x=2906.87f+ix*0.25f,y=831.485f+iy*0.25f,z=515.16f;
                    float upperFloor=ship.m_model->getHeight(x,y,z,3.f,ship.GetPhases(),false),lowerFloor=lower.m_model->getHeight(x,y,z,3.f,ship.GetPhases(),false);
                    GameObject* expected=upperFloor>=lowerFloor?static_cast<GameObject*>(&ship):&lower;float nearest=std::max(upperFloor,lowerFloor);
                    DynamicTreeCallback receipt;float actual=tree.getHeight(x,y,z,3.f,ship.GetPhases(),false,&receipt);
#ifndef DYNAMIC_MODEL_NEAREST_BASELINE
                    Check(std::fabs(actual-nearest)<0.001f&&receipt.go==expected,"Object insertion order selected a farther supporting model");
#endif
                    G3D::Vector3 from(x,y,z+0.5f),to(x,y,z-2.5f),hit;DynamicTreeCallback contact;
                    Check(tree.getObjectHitPos(ship.GetPhases(),false,from,to,hit,0.f,&contact),"Overlapping native contact missing");
#ifndef DYNAMIC_MODEL_NEAREST_BASELINE
                    Check(std::fabs(hit.z-nearest)<0.001f&&contact.go==expected,"Contact selected a farther model in the same grid node");
#endif
                    DynamicTreeCallback filter;filter.filter=[&](GameObject const* object){return object==&lower;};
                    float allowed=tree.getHeight(x,y,z,3.f,ship.GetPhases(),false,&filter);
#ifndef DYNAMIC_MODEL_NEAREST_BASELINE
                    Check(std::fabs(allowed-lowerFloor)<0.001f&&filter.go==&lower,"Closest-surface search bypassed caller model filter");
#endif
                }
                tree.remove(*ship.m_model);tree.remove(*lower.m_model);
            }
            lower.ResetMap();
        }
        std::cout<<"PASS "<<checked<<" native closest-surface checks;matched="<<matched<<" first-hit mismatches="<<different<<'\n';
        CharacterDatabase.Close();LoginDatabase.Close();WorldDatabase.Close();HotfixDatabase.Close();MySQL::Library_End();return 0;
    }catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}
}
