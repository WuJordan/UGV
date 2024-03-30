#include <ControllerInterface.h>
#include "Controller.h"
#include "Laser.h"
#include "TMM.h"
#include "Display.h"
#include "NetworkedModule.h"
#include "GNSS.h"
#include "VC.h"
#include <conio.h>

using namespace System;
using namespace System::Threading;

int main() {
	ThreadManagement^ TMT = gcnew ThreadManagement;
	TMT->threadFunction();
	Console::ReadKey();
}

