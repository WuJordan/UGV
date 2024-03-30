#using <System.dll>

#include "Display.h"

Display::Display(SM_ThreadManagement^ SM_TM, SM_Laser^ SM_Laser) {
	SM_TM_ = SM_TM;
	SM_Laser_ = SM_Laser;
	Watch = gcnew Stopwatch;
}

// Send/Recieve data from shared memory structures
error_state Display::processSharedMemory() {
	return SUCCESS;
}

void Display::shutdownModules() {
	Monitor::Enter(SM_TM_->lockObject);
	SM_TM_->shutdown = bit_ALL;
	Monitor::Exit(SM_TM_->lockObject);
}

bool Display::getShutdownFlag() {
	return (SM_TM_->shutdown & bit_DISPLAY);
}

void Display::threadFunction() {

	SM_TM_->ThreadBarrier->SignalAndWait();
	Watch->Start();
	if (connect("127.0.0.1", 28000) == SUCCESS) {
		while (!getShutdownFlag()) {
			communicate();
			processHeartbeats();
			Thread::Sleep(500);
		}
	};

	Console::WriteLine("Exiting Display");
}

error_state Display::processHeartbeats() {
	Monitor::Enter(SM_TM_->lockObject);
	if ((SM_TM_->heartbeat & bit_DISPLAY) == 0) {
		SM_TM_->heartbeat |= bit_DISPLAY;
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


error_state Display::connect(String^ hostName, int portNumber) {
	try {
		Client = gcnew TcpClient(hostName, portNumber);
		Stream = Client->GetStream();
		Client->NoDelay = true;
		Client->ReceiveTimeout = 2500;
		Client->SendTimeout = 2500;
		Client->ReceiveBufferSize = 1024;
		Client->SendBufferSize = 1024;

		ReadData = gcnew array<unsigned char>(5000);
	}
	catch (...) {
		return ERR_CONNECTION;
	}
	return SUCCESS;
}

error_state Display::communicate() {
	sendDisplayData();
	return SUCCESS;
}

void Display::sendDisplayData() {
	// Serialize the data arrays to a byte array
	
	// Lock SM_Laser:
	Monitor::Enter(SM_Laser_->lockObject);

	array<Byte>^ dataX =
		gcnew array<Byte>(SM_Laser_->x->Length * sizeof(double));
	Buffer::BlockCopy(SM_Laser_->x, 0, dataX, 0, dataX->Length);
	array<Byte>^ dataY =
		gcnew array<Byte>(SM_Laser_->y->Length * sizeof(double));
	Buffer::BlockCopy(SM_Laser_->y, 0, dataY, 0, dataY->Length);

	// unlock SM_Laser:
	Monitor::Exit(SM_Laser_->lockObject);

	// Send byte data over connection
	Stream->Write(dataX, 0, dataX->Length);
	Thread::Sleep(10);
	Stream->Write(dataY, 0, dataY->Length);


}