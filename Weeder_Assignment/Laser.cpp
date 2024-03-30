#using <System.dll>
#include "Laser.h"

#define EXPECTED_FRAGMENTS 393

Laser::Laser(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser) {
	SM_TM_ = SM_TM;
	SM_Laser_ = SM_Laser;
	Watch = gcnew Stopwatch;
} 


error_state Laser::processSharedMemory() {
	// Lock SM_Laser:
	Monitor::Enter(SM_Laser_->lockObject);

	for (int i = 0; i < Range->Length; i++) {
		SM_Laser_->x[i] = RangeX[i];
		SM_Laser_->y[i] = RangeY[i];
	}

	// unlock SM_Laser:
	Monitor::Exit(SM_Laser_->lockObject);
	return SUCCESS;
}

void Laser::shutdownModules() {
	Monitor::Enter(SM_TM_->lockObject);
	SM_TM_->shutdown = bit_ALL;
	Monitor::Exit(SM_TM_->lockObject);
}

bool Laser::getShutdownFlag() {
	return (SM_TM_->shutdown & bit_LASER);
}

void Laser::threadFunction() {

	SM_TM_->ThreadBarrier->SignalAndWait();
	Watch->Start();
	if (connect(WEEDER_ADDRESS, 23000) == SUCCESS && authenticate() == SUCCESS) {
		while (!getShutdownFlag()) {
			if (communicate() == SUCCESS) {
				processSharedMemory();
			}
			Thread::Sleep(100);
			processHeartbeats();
		}
	}

	Console::WriteLine("Exiting Laser");
}

error_state Laser::processHeartbeats() {
	Monitor::Enter(SM_TM_->lockObject);
	if ((SM_TM_->heartbeat & bit_LASER) == 0) {
		SM_TM_->heartbeat |= bit_LASER;
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

error_state Laser::connect(String^ hostName, int portNumber) {
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

error_state Laser::communicate() {

	String^ Command = "sRN LMDscandata";
	SendData = Encoding::ASCII->GetBytes(Command);

	Stream->WriteByte(0x02); 
	Stream->Write(SendData, 0, SendData->Length);
	Stream->WriteByte(0x03); 

	System::Threading::Thread::Sleep(100);
	Stream->Read(ReadData, 0, ReadData->Length);
	String^ Response = Encoding::ASCII->GetString(ReadData);

	// Split into substrings seperated by spaces:
	array<wchar_t>^ space = {' '};
	array<String^>^ StringArray = Response->Split(space);


	double StartAngle = System::Convert::ToInt32(StringArray[23], 16);
	double Resolution = System::Convert::ToInt32(StringArray[24], 16)/10000.0;
	int numRange = System::Convert::ToInt32(StringArray[25], 16);
	
	if (StringArray->Length != EXPECTED_FRAGMENTS || numRange != STANDARD_LASER_LENGTH) {
		return ERR_INVALID_DATA;
	}

	Range = gcnew array<double>(numRange);
	RangeX = gcnew array<double>(numRange); 
	RangeY = gcnew array<double>(numRange); 

	try {
		for (int i = 0; i < numRange; i++) {
			Range[i] = System::Convert::ToInt32(StringArray[26 + i], 16);
			RangeY[i] = Range[i] * Math::Sin(i * Resolution * (Math::PI / 180.0));
			RangeX[i] = Range[i] * Math::Cos(i * Resolution * (Math::PI / 180.0));
		}
	}
	catch (...) {
		Console::WriteLine("Invalid Laser Data");
		return ERR_INVALID_DATA;
	}
	
	return SUCCESS;
}

error_state Laser::authenticate() {
	String^ zID = "5360470\n";
	SendData = Encoding::ASCII->GetBytes(zID);
	Stream->Write(SendData, 0, SendData->Length);

	System::Threading::Thread::Sleep(500);
	Stream->Read(ReadData, 0, ReadData->Length);
	String^ Response = Encoding::ASCII->GetString(ReadData); 
	
	// Make sure it recieves "OK\n"
	if (Response->Contains("OK")) {
		Console::WriteLine("Laser Authenticated Successfully");
		return SUCCESS;
	}
	else {
		Console::WriteLine("Laser Authentication Failed");
		return ERR_CONNECTION;
	}
}