#pragma once

#include "SMObjects.h"
#include <NetworkedModule.h>



ref class Display : public NetworkedModule {
public:
    Display(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser); 

    error_state processSharedMemory() override;

    void shutdownModules();

    bool getShutdownFlag() override;

    void threadFunction() override;

    error_state processHeartbeats();

    error_state connect(String^ hostName, int portNumber) override;

    error_state communicate() override;

    void sendDisplayData();

private:
    Stopwatch^ Watch;
    SM_ThreadManagement^ SM_TM_;
    SM_Laser^ SM_Laser_;
};