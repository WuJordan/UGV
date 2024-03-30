#include <ControllerInterface.h>
#using <System.dll>

#include "Controller.h"
#include <stdexcept>

Controller::Controller(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC) {
	SM_TM_ = SM_TM;
	SM_VC_ = SM_VC;
	con = new ControllerInterface(1, 0);
	Watch = gcnew Stopwatch;
}

// Send to Shared Memory
error_state Controller::processSharedMemory() {
	Monitor::Enter(SM_VC_->lockObject);

	SM_VC_->Speed = calculatedSpeed;
	SM_VC_->Steering = calculatedSteering;

	Monitor::Exit(SM_VC_->lockObject);
	return SUCCESS;
}

error_state Controller::processHeartbeats() {
	Monitor::Enter(SM_TM_->lockObject);
	if ((SM_TM_->heartbeat & bit_CONTROLLER) == 0) {
		SM_TM_->heartbeat |= bit_CONTROLLER;
		Watch->Restart();
	}
	else {
		if (Watch->ElapsedMilliseconds > CRASH_LIMIT) {
			shutdownModules();
			return ERR_CRITICAL_PROCESS_FAILURE;
		}
	}
	Monitor::Exit(SM_TM_->lockObject);
	return SUCCESS;
}

void Controller::threadFunction() {
	SM_TM_->ThreadBarrier->SignalAndWait();
	Watch->Start();

	while (!getShutdownFlag()) {
		if (con->GetState().buttonA || !con->IsConnected()) {
			Console::WriteLine("A Pressed or Controller Disconnected: Shutting Down");
			haltRobot();
			Thread::Sleep(500);
			shutdownModules();
		} else {
			controlSpeed();
			controlSteering();
		}
		
		processHeartbeats();

		if (checkData() == SUCCESS) {
			processSharedMemory();
		}
	}
	haltRobot();
	delete con;
	Console::WriteLine("Exiting Controller");
}

bool Controller::getShutdownFlag() {
	return (SM_TM_->shutdown & bit_CONTROLLER);
}

void Controller::controlSpeed() {
	calculatedSpeed = (con->GetState().rightTrigger - con->GetState().leftTrigger);
}
void Controller::controlSteering() {
	calculatedSteering = -(con->GetState().rightThumbX * 40);

}
void Controller::shutdownModules() {
	Monitor::Enter(SM_TM_->lockObject);
	SM_TM_->shutdown = bit_ALL;
	Monitor::Exit(SM_TM_->lockObject);
}

error_state Controller::checkData() {
	if (calculatedSpeed > 1 || calculatedSpeed < -1) {
		return ERR_INVALID_DATA;
	}

	if (calculatedSteering > 40 || calculatedSteering < -40) {
		return ERR_INVALID_DATA;
	}

	return SUCCESS;
};

void Controller::haltRobot() {
	calculatedSpeed = 0;
	calculatedSteering = 0;
	processSharedMemory();
}