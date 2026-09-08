#include "settings_loader.h"
int main()
{
	Refactoring::SettingsLoader ss;

	ss.Init();
	ss.LoadSettings();

	return 0;
}