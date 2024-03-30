#using <System.dll>

#include "VC.h"
#include <sstream>

VehicleControl::VehicleControl(SM_ThreadManagement^ SM_TM, SM_VehicleControl^ SM_VC) {
	SM_TM_ = SM_TM;
	SM_VC_ = SM_VC;
	msgflag = false;
	Watch = gcnew Stopwatch;
}

error_state VehicleControl::processSharedMemory() {
	// Lock VC SM
	Monitor::Enter(SM_VC_->lockObject);

	speed = SM_VC_->Speed;
	steering = SM_VC_->Steering;

	// Unlock VC SM
	Monitor::Exit(SM_VC_->lockObject);
	return SUCCESS;
}

void VehicleControl::shutdownModules() {
	Monitor::Enter(SM_TM_->lockObject);
	SM_TM_->shutdown = bit_ALL;
	Monitor::Exit(SM_TM_->lockObject);
}

bool VehicleControl::getShutdownFlag() {
	return (SM_TM_->shutdown & bit_VC);
}

void VehicleControl::threadFunction() {
	SM_TM_->ThreadBarrier->SignalAndWait();
	Watch->Start();
	if (connect(WEEDER_ADDRESS, 25000) == SUCCESS && authenticate() == SUCCESS) {
		while (!getShutdownFlag()) {
			processSharedMemory();
			communicate();
			Thread::Sleep(100);
			processHeartbeats();
		}
	};
	//authenticate();

	Console::WriteLine("Exiting VC");
}

error_state VehicleControl::processHeartbeats() {
	Monitor::Enter(SM_TM_->lockObject);
	if ((SM_TM_->heartbeat & bit_VC) == 0) { 
		SM_TM_->heartbeat |= bit_VC;
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

String^ VehicleControl::formatCommand(double speed, double steer) {
	//convert params into following ASCII String form : # <steer> <speed> <flag> #
	int newflag = 1;
	if (msgflag) {
		newflag = 0;
	}

	String^ formattedString = String::Format("# {0} {1} {2} #", steer, speed, newflag);
	return formattedString;
}


error_state VehicleControl::connect(String^ hostName, int portNumber) {
	// 25000 for VC Service
	try {
		Client = gcnew TcpClient(hostName, portNumber);
		Stream = Client->GetStream();
		Client->NoDelay = true;
		Client->ReceiveTimeout = 2500;
		Client->SendTimeout = 2500;
		Client->ReceiveBufferSize = 1024;
		Client->SendBufferSize = 1024;

		ReadData = gcnew array<unsigned char>(5000);
		SendData = gcnew array<unsigned char>(64);
	}
	catch (...) {
		return ERR_CONNECTION;
	}
	return SUCCESS;
}

error_state VehicleControl::communicate() {
	String^ Command = formatCommand(speed, steering);

	// Send command 
	SendData = Encoding::ASCII->GetBytes(Command);
	Stream->Write(SendData, 0, SendData->Length);

	// Toggle flag according to spec
	bool newflag = !msgflag;
	msgflag = newflag;

	return SUCCESS;
}

error_state VehicleControl::authenticate() {
	String^ zID = "5360470\n";
	SendData = Encoding::ASCII->GetBytes(zID);
	Stream->Write(SendData, 0, SendData->Length);

	System::Threading::Thread::Sleep(100);
	Stream->Read(ReadData, 0, ReadData->Length);
	String^ Response = Encoding::ASCII->GetString(ReadData); 
	
	// Make sure it recieves "OK\n"
	if (Response->Contains("OK")) {
		Console::WriteLine("Vehicle Control Authenticated Successfully");
		return SUCCESS;
	}
	else {
		Console::WriteLine("Vehicle Control Authentication Failed");
		return ERR_CONNECTION;
	}
}