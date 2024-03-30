#pragma once

#include "SMObjects.h"
#include <NetworkedModule.h>


ref class VehicleControl : public NetworkedModule {
public:
    VehicleControl(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC); 

    error_state processSharedMemory() override;

    void shutdownModules();

    bool getShutdownFlag() override;

    void threadFunction() override;

    error_state processHeartbeats();

    String^ formatCommand(double speed, double steer);

    error_state connect(String^ hostName, int portNumber) override;

    error_state communicate() override;

    error_state authenticate();

private:
    Stopwatch^ Watch;
    array<unsigned char>^ SendData;
    SM_ThreadManagement^ SM_TM_;
    SM_VehicleControl^ SM_VC_;
    double speed;
    double steering;
    bool msgflag;
};