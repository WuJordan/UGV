#pragma once

#include <UGVModule.h>
#include <SMObjects.h>

ref struct ThreadProperties {

    ThreadStart^ ThreadStart_;
    bool Critical;
    String^ ThreadName;
    uint8_t BitID;

    ThreadProperties(ThreadStart^ start, String^ name, uint8_t id, bool critical) {
        ThreadStart_ = start;
        ThreadName = name;
        BitID = id;
        Critical = critical;
    }
};

ref class ThreadManagement : public UGVModule {
public:
    error_state setupSharedMemory();

    error_state processSharedMemory() override;

    void shutdownModules();

    void shutdownController();

    bool getShutdownFlag() override;

    void threadFunction() override;

    error_state processHeartbeats(); 

    SM_Laser^ getLaserSM();
    SM_ThreadManagement^ getTMSM();
    SM_GPS^ getGPSSM();
    SM_VehicleControl^ getVehicleControlSM();

private:
    array<Thread^>^ threadList;
    array<ThreadProperties^>^ threadPropertiesList;
    SM_Laser^ SM_Laser_;
    SM_GPS^ SM_GPS_;
    SM_VehicleControl^ SM_VC_;
};
