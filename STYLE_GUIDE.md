# Contributing conventions summary

| What	| Convention	| Example |
| --- | --- | --- |
| local variables	| snake_case	| monitor_modes |
| class members |	snake_case	| m_wdf_device (with m_ prefix) |
| global variables | grouped in struct |	g_settings.hdr_plus |
| our functions |	snake_case	| load_edid(), init_adapter() |
| framework callbacks | PascalCase (do not touch)	| VirtualDisplayDriverDeviceAdd |
| classes and structs | PascalCase	| DriverSettings, IndirectDeviceContext |
| Constants | UPPER_CASE	| PIPE_NAME |
