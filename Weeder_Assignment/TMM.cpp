#include "ControllerInterface.h"
#include "Controller.h"
#using <System.dll>
#include <conio.h>

#include "TMM.h"
#include "Laser.h"
#include "GNSS.h"
#include "VC.h"
#include "Display.h"
#include <UGVModule.h>


error_state ThreadManagement::setupSharedMemory() {
    this->SM_Laser_ = gcnew SM_Laser;
    this->SM_GPS_ = gcnew SM_GPS;
    this->SM_VC_ = gcnew SM_VehicleControl;
    this->SM_TM_ = gcnew SM_ThreadManagement;
    if (this->SM_Laser_ == nullptr || this->SM_GPS_ == nullptr || this->SM_VC_ == nullptr || this->SM_TM_ == nullptr) {
        return ERR_STARTUP;
    }
    return SUCCESS;
}

error_state ThreadManagement::processSharedMemory() {

    try {
        // Initialise ThreadPropertiesLIst
        threadPropertiesList = gcnew array<ThreadProperties^>{
            gcnew ThreadProperties(gcnew ThreadStart(gcnew Laser(SM_TM_, SM_Laser_), &Laser::threadFunction), "Laser", bit_LASER, true),
            gcnew ThreadProperties(gcnew ThreadStart(gcnew GNSS(SM_TM_, SM_GPS_), &GNSS::threadFunction), "GNSS", bit_GPS, false),
            gcnew ThreadProperties(gcnew ThreadStart(gcnew Controller(SM_TM_, SM_VC_), &Controller::threadFunction), "Controller", bit_CONTROLLER, true),
            gcnew ThreadProperties(gcnew ThreadStart(gcnew VehicleControl(SM_TM_, SM_VC_), &VehicleControl::threadFunction), "Vehicle Control", bit_VC, true),
            gcnew ThreadProperties(gcnew ThreadStart(gcnew Display(SM_TM_, SM_Laser_), &Display::threadFunction), "Display", bit_DISPLAY, true),
        };

        //Initialise ThreadsList  
        threadList = gcnew array<Thread^>(threadPropertiesList->Length);

        // Initialise stopwatchList
        SM_TM_->WatchList = gcnew array<Stopwatch^>(threadPropertiesList->Length);

        // Make thread Barrier 
        SM_TM_->ThreadBarrier = gcnew Barrier(threadPropertiesList->Length + 1);

        // Start all Threads:
        for (int i = 0; i < threadPropertiesList->Length; i++) {
            SM_TM_->WatchList[i] = gcnew Stopwatch;
            threadList[i] = gcnew Thread(threadPropertiesList[i]->ThreadStart_);
            threadList[i]->Start();
        }
    
    }
    catch (...) {
        return ERR_STARTUP;
    }
    return SUCCESS;
}

void ThreadManagement::shutdownModules() {
    Monitor::Enter(SM_TM_->lockObject);
    SM_TM_->shutdown = bit_ALL;
    Monitor::Exit(SM_TM_->lockObject);
}

void ThreadManagement::shutdownController() {
    Monitor::Enter(SM_TM_->lockObject);
    SM_TM_->shutdown = bit_CONTROLLER;
    Monitor::Exit(SM_TM_->lockObject);
}


bool ThreadManagement::getShutdownFlag() {
    return (SM_TM_->shutdown & bit_TM);
}

void ThreadManagement::threadFunction() {
    setupSharedMemory();
    processSharedMemory();

    SM_TM_->ThreadBarrier->SignalAndWait();

    // Start all StopWatches
    for (int i = 0; i < threadList->Length; i++) {
        SM_TM_->WatchList[i]->Start();
    }

    while (!getShutdownFlag()) {
        if (_kbhit()) {
            // Capture the key without consuming it.
            char key = _getch(); 
            if (key == 'q' || key == 'Q') {
                Console::WriteLine("Q Pressed: Shutting Down");
                // Shutdown controller first to halt robot
                shutdownController();
                Thread::Sleep(100);
                shutdownModules(); 
                break; 
            }
        }
        if (processHeartbeats() != SUCCESS) {
            break;
        }
        Thread::Sleep(50);

    }

    for (int i = 0; i < threadPropertiesList->Length; i++) {
        threadList[i]->Join();
    }

    Console::WriteLine("Exiting TMM");

}

error_state ThreadManagement::processHeartbeats() {
    Monitor::Enter(SM_TM_->lockObject);
    for (int i = 0; i < threadList->Length; i++) {
        if (SM_TM_->heartbeat & threadPropertiesList[i]->BitID) { 
            SM_TM_->heartbeat ^= threadPropertiesList[i]->BitID;
            SM_TM_->WatchList[i]->Restart();
        }
        else {
            if (SM_TM_->WatchList[i]->ElapsedMilliseconds > CRASH_LIMIT) {
                if (threadPropertiesList[i]->Critical) {
                    //Shutdown All
                    Console::WriteLine("Critical Thread Failure: " + threadPropertiesList[i]->ThreadName);
                    shutdownModules();
                    return ERR_CRITICAL_PROCESS_FAILURE;
                }
                else {
                    Console::WriteLine("Restarting Thread: " + threadPropertiesList[i]->ThreadName);
                    threadList[i] = gcnew Thread(threadPropertiesList[i]->ThreadStart_);
                    SM_TM_->ThreadBarrier = gcnew Barrier(1);
                    threadList[i]->Start();
                }
            }
        }
    }
    Monitor::Exit(SM_TM_->lockObject);
    return SUCCESS;
}

SM_Laser^ ThreadManagement::getLaserSM() {
    return SM_Laser_;
}
SM_ThreadManagement^ ThreadManagement::getTMSM() {
    return SM_TM_;
}
SM_GPS^ ThreadManagement::getGPSSM() {
    return SM_GPS_;
}
SM_VehicleControl^ ThreadManagement::getVehicleControlSM() {
    return SM_VC_;
}