#pragma once

#include "SMObjects.h"
#include <NetworkedModule.h>

#define CRC32_POLYNOMIAL 0xEDB88320L

ref class GNSS : public NetworkedModule {
public:
    GNSS(SM_ThreadManagement^ SM_TM, SM_GPS^ SM_GPS_); 

    error_state processSharedMemory() override;

    void shutdownModules();

    bool getShutdownFlag() override;

    void threadFunction() override;

    error_state processHeartbeats();

    error_state checkCRC();

    error_state connect(String^ hostName, int portNumber) override;

    error_state communicate() override;

    unsigned long CRC32Value(int i);

    unsigned long CalculateBlockCRC32(unsigned long ulCount,
        unsigned char* ucBuffer);

private:
    Stopwatch^ Watch;
    SM_ThreadManagement^ SM_TM_;
    SM_GPS^ SM_GPS_;
    double Northing, Easting, Height;
    unsigned int CRC;
};