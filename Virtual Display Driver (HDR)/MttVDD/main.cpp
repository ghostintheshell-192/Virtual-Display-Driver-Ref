#include "logger.h"
#include "settings_loader.h"

int main()
{
	HANDLE hPipe;

	// for now, hard-coded values are passed
	Refactoring::Logger log("C:\\VirtualDisplayDriver\\", true, true, true);

	log.Init(&hPipe);

	Refactoring::SettingsLoader ss(&log);

	ss.Init();
	ss.LoadSettings();

	return 0;
}