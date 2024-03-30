#pragma once

#include "SMObjects.h"
#include <NetworkedModule.h>

ref class Laser : public NetworkedModule {
public:

	Laser(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser); 

    error_state processSharedMemory() override;

    void shutdownModules();

    bool getShutdownFlag() override;

    void threadFunction() override;

    error_state processHeartbeats();

    error_state connect(String^ hostName, int portNumber) override;

    error_state communicate() override;

    error_state authenticate();

private:
    Stopwatch^ Watch;
    array<unsigned char>^ SendData;
    SM_ThreadManagement^ SM_TM_;
    SM_Laser^ SM_Laser_;
    array<double>^ Range; 
    array<double>^ RangeX; 
    array<double>^ RangeY;
};