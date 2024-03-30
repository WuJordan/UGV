#using <System.dll>

#include "GNSS.h"

GNSS::GNSS(SM_ThreadManagement^ SM_TM, SM_GPS^ SM_GPS) {
	SM_TM_ = SM_TM;
	SM_GPS_ = SM_GPS;
	Watch = gcnew Stopwatch;
}

// Send/Recieve data from shared memory structures
error_state GNSS::processSharedMemory() {
	Monitor::Enter(SM_GPS_->lockObject);
	SM_GPS_->Height = Height;
	SM_GPS_->Northing = Northing;
	SM_GPS_->Easting = Easting;
	Monitor::Exit(SM_GPS_->lockObject);
	return SUCCESS;
}

void GNSS::shutdownModules() {
	Monitor::Enter(SM_TM_->lockObject);
	SM_TM_->shutdown = bit_ALL;
	Monitor::Exit(SM_TM_->lockObject);
}

bool GNSS::getShutdownFlag() {
	return (SM_TM_->shutdown & bit_GPS);
}

void GNSS::threadFunction() {
	SM_TM_->ThreadBarrier->SignalAndWait();
	Watch->Start();
	// Connect to GPS
	if (connect(WEEDER_ADDRESS, 24000) == SUCCESS) {
		while (!getShutdownFlag()) {
			if (communicate() == SUCCESS) {
				processSharedMemory();
			}
			else {
				Console::WriteLine("Invalid GNSS Data");
			};
			processHeartbeats();
			Thread::Sleep(500);
		}
	};


	Console::WriteLine("Exiting GNSS");
}

error_state GNSS::processHeartbeats() {
	Monitor::Enter(SM_TM_->lockObject);
	if ((SM_TM_->heartbeat & bit_GPS) == 0) {
		SM_TM_->heartbeat |= bit_GPS;
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

error_state GNSS::checkCRC() {
	// Including Header but Not including CRC (last 4 bytes)
	unsigned char GNSSData[108]; 
	for (int i = 0; i < 108; i++) {
		GNSSData[i] = ReadData[i];
	}

	unsigned long calculated = CalculateBlockCRC32(108, GNSSData);
	if (CRC == calculated) {
		return SUCCESS;
	}
	return ERR_INVALID_DATA;
}

error_state GNSS::connect(String^ hostName, int portNumber) {
	// connect to the GPS -> Port 24000
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

error_state GNSS::communicate() {
	unsigned int Header = 0; // 4 bytes
	Byte data;
	while (Header != 0xaa44121c) {
		data = Stream->ReadByte();
		Header = (Header << 8) | data;
	}
	
	// Store header into ReadData for CRC checking
	unsigned int mask = 0xFF000000;
	int j = 3;
	for (int i = 0; i < 4; i++) {
		ReadData[i] = (Header & mask) >> (8 * j);
		mask = mask >> 8;
		j--;
	}

	// Store GNSS Data into ReadData
	for (int i = 4; i < 112; i++) {
		ReadData[i] = Stream->ReadByte();
	}

	Northing = BitConverter::ToDouble(ReadData, 44);
	Easting = BitConverter::ToDouble(ReadData, 52);
	Height = BitConverter::ToDouble(ReadData, 60);
	CRC = BitConverter::ToInt32(ReadData, 108);

	Console::WriteLine("[GNSS] N:{0:F3} E:{1:F3} H:{2:F3} CRC:0x{3:X}", Northing, Easting, Height, CRC);

	return checkCRC();
}

unsigned long GNSS::CRC32Value(int i)
{
	int j;
	unsigned long ulCRC;
	ulCRC = i;
	for (j = 8; j > 0; j--)
	{
		if (ulCRC & 1)
			ulCRC = (ulCRC >> 1) ^ CRC32_POLYNOMIAL;
		else
			ulCRC >>= 1;
	}
	return ulCRC;
}
unsigned long GNSS::CalculateBlockCRC32(
	unsigned long ulCount, /* Number of bytes in the data block */
	unsigned char* ucBuffer) /* Data block */
{
	unsigned long ulTemp1;
	unsigned long ulTemp2;
	unsigned long ulCRC = 0;
	while (ulCount-- != 0)
	{
		ulTemp1 = (ulCRC >> 8) & 0x00FFFFFFL;
		ulTemp2 = CRC32Value(((int)ulCRC ^ *ucBuffer++) & 0xff);
		ulCRC = ulTemp1 ^ ulTemp2;
	}
	return(ulCRC);
}


