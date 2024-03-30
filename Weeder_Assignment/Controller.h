#pragma once

#include <UGVModule.h>
#include <SMObjects.h>
#include <ControllerInterface.h>



ref class Controller : public UGVModule {
public:

	Controller(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC);

	error_state processSharedMemory() override;

	error_state processHeartbeats();

	void threadFunction() override;
	bool getShutdownFlag() override;

	void controlSpeed();
	void controlSteering();
	void shutdownModules();
	error_state checkData();
	void haltRobot();

private:
	Stopwatch^ Watch;
	SM_ThreadManagement^ SM_TM_;
	SM_VehicleControl^ SM_VC_;
	ControllerInterface* con;
	double calculatedSpeed;
	double calculatedSteering;
};