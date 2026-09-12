/*++

Copyright (c) Microsoft Corporation

Abstract:

	MSDN documentation on indirect displays can be found at https://msdn.microsoft.com/en-us/library/windows/hardware/mt761968(v=vs.85).aspx.

Environment:

	User Mode, UMDF

--*/

#include "driver.h"
#include "utilities.h"
#include "settings_loader.h"
//#include "Driver.tmh"
#include<fstream>
#include<sstream>
#include<string>
#include<tuple>
#include<vector>
#include<algorithm>
#include<iomanip>
#include<chrono>
#include <AdapterOption.h>
#include <xmllite.h>
#include <shlwapi.h>
#include <atlbase.h>
#include <iostream>
#include <cstdlib>
#include <windows.h>
#include <cstdio>
#include <sddl.h>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <cerrno>
#include <locale>
#include <cwchar>
#include <map>
#include <set>
#include <functional>


#define PIPE_NAME L"\\\\.\\pipe\\MTTVirtualDisplayPipe"

#pragma comment(lib, "xmllite.lib")
#pragma comment(lib, "shlwapi.lib")

HANDLE hPipeThread = NULL;
bool g_Running = true;
mutex g_Mutex;
HANDLE g_pipeHandle = INVALID_HANDLE_VALUE;

using namespace std;
using namespace Microsoft::IndirectDisp;
using namespace Microsoft::WRL;

extern "C" DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD VirtualDisplayDriverDeviceAdd;
EVT_WDF_DEVICE_D0_ENTRY VirtualDisplayDriverDeviceD0Entry;

EVT_IDD_CX_ADAPTER_INIT_FINISHED VirtualDisplayDriverAdapterInitFinished;
EVT_IDD_CX_ADAPTER_COMMIT_MODES VirtualDisplayDriverAdapterCommitModes;

EVT_IDD_CX_PARSE_MONITOR_DESCRIPTION VirtualDisplayDriverParseMonitorDescription;
EVT_IDD_CX_MONITOR_GET_DEFAULT_DESCRIPTION_MODES VirtualDisplayDriverMonitorGetDefaultModes;
EVT_IDD_CX_MONITOR_QUERY_TARGET_MODES VirtualDisplayDriverMonitorQueryModes;

EVT_IDD_CX_MONITOR_ASSIGN_SWAPCHAIN VirtualDisplayDriverMonitorAssignSwapChain;
EVT_IDD_CX_MONITOR_UNASSIGN_SWAPCHAIN VirtualDisplayDriverMonitorUnassignSwapChain;

EVT_IDD_CX_ADAPTER_QUERY_TARGET_INFO VirtualDisplayDriverEvtIddCxAdapterQueryTargetInfo;
EVT_IDD_CX_MONITOR_SET_DEFAULT_HDR_METADATA VirtualDisplayDriverEvtIddCxMonitorSetDefaultHdrMetadata;
EVT_IDD_CX_PARSE_MONITOR_DESCRIPTION2 VirtualDisplayDriverEvtIddCxParseMonitorDescription2;
EVT_IDD_CX_MONITOR_QUERY_TARGET_MODES2 VirtualDisplayDriverEvtIddCxMonitorQueryTargetModes2;
EVT_IDD_CX_ADAPTER_COMMIT_MODES2 VirtualDisplayDriverEvtIddCxAdapterCommitModes2;

EVT_IDD_CX_MONITOR_SET_GAMMA_RAMP VirtualDisplayDriverEvtIddCxMonitorSetGammaRamp;

Refactoring::DriverSettings g_settings;
Refactoring::ColourSettingsIDDCX g_colours_iddcx;
Refactoring::CursorSettingsIDDCX g_cursor_iddcx;

Refactoring::MonitorProfile g_default_profile;
Refactoring::MonitorProfile g_custom_profile;

Refactoring::Logger g_log("C:\\VirtualDisplayDriver", true, false, true);

Refactoring::SettingsLoader g_settings_manager(&g_log, &g_settings);

struct
{
	AdapterOption Adapter;
} Options;
vector<Refactoring::Resolution> monitorModes;
vector< DISPLAYCONFIG_VIDEO_SIGNAL_INFO> s_KnownMonitorModes2;
UINT numVirtualDisplays;
wstring gpuname;
wstring confpath = L"C:\\VirtualDisplayDriver";

constexpr DISPLAYCONFIG_VIDEO_SIGNAL_INFO dispinfo(UINT32 h, UINT32 v, UINT32 rn, UINT32 rd);

namespace
{
	void RebuildKnownMonitorModesCache()
	{
		s_KnownMonitorModes2.clear();
		s_KnownMonitorModes2.reserve(monitorModes.size());

		for (const auto& mode : monitorModes)
		{
			s_KnownMonitorModes2.push_back(
				dispinfo(
					mode.width,
					mode.height,
					mode.refresh_num,
					mode.refresh_den));
		}
	}
}

const char* XorCursorSupportLevelToString(IDDCX_XOR_CURSOR_SUPPORT level) {
	switch (level) {
	case IDDCX_XOR_CURSOR_SUPPORT_UNINITIALIZED:
		return "IDDCX_XOR_CURSOR_SUPPORT_UNINITIALIZED";
	case IDDCX_XOR_CURSOR_SUPPORT_NONE:
		return "IDDCX_XOR_CURSOR_SUPPORT_NONE";
	case IDDCX_XOR_CURSOR_SUPPORT_FULL:
		return "IDDCX_XOR_CURSOR_SUPPORT_FULL";
	case IDDCX_XOR_CURSOR_SUPPORT_EMULATION:
		return "IDDCX_XOR_CURSOR_SUPPORT_EMULATION";
	default:
		return "Unknown";
	}
}

vector<unsigned char> Microsoft::IndirectDisp::IndirectDeviceContext::s_KnownMonitorEdid; //Changed to support static vector

std::map<LUID, std::shared_ptr<Direct3DDevice>, Microsoft::IndirectDisp::LuidComparator> Microsoft::IndirectDisp::IndirectDeviceContext::s_DeviceCache;
std::mutex Microsoft::IndirectDisp::IndirectDeviceContext::s_DeviceCacheMutex;

struct IndirectDeviceContextWrapper
{
	IndirectDeviceContext* pContext;

	void Cleanup()
	{
		delete pContext;
		pContext = nullptr;
	}
};




// Find and validate preferred mode from EDID
Refactoring::Resolution FindPreferredModeFromEdid(const Refactoring::MonitorProfile &profile, const vector<Refactoring::Resolution> &availableModes)
{
	// Default fallback mode
	Refactoring::Resolution preferredMode =
		Refactoring::Resolution(
			g_settings.auto_resolutions.preferred_mode.fallback_width, 
			g_settings.auto_resolutions.preferred_mode.fallback_height,
			1000, 
			g_settings.auto_resolutions.preferred_mode.fallback_refresh);

	if (!g_settings.auto_resolutions.preferred_mode.preferred)
	{
		g_log.Message(Refactoring::LogType::Info, "EDID preferred mode disabled, using fallback");
		return preferredMode;
	}

	// Look for EDID preferred mode in available modes
	for (const auto &mode : availableModes)
	{
		if (mode.width == profile.preferred_res.width && mode.height == profile.preferred_res.height)
		{
			// Found matching resolution, use it
			preferredMode = mode;
			g_log.Message(
				Refactoring::LogType::Info,
				std::format("Found EDID preferred mode: {}x{} @ {} Hz", profile.preferred_res.width, profile.preferred_res.height, mode.refresh_den).c_str());
			break;
		}
	}

	return preferredMode;
}

int gcd(int a, int b) {
	while (b != 0) {
		int temp = b;
		b = a % b;
		a = temp;
	}
	return a;
}

void float_to_vsync(float refresh_rate, int& num, int& den) {
	den = 10000;

	num = static_cast<int>(round(refresh_rate * den));

	int divisor = gcd(num, den);
	num /= divisor;
	den /= divisor;
}

void LogIddCxVersion() {
	IDARG_OUT_GETVERSION outArgs;
	NTSTATUS status = IddCxGetVersion(&outArgs);

	if (NT_SUCCESS(status)) {
		//char versionStr[16];
		//sprintf_s(versionStr, "0x%lx", outArgs.IddCxVersion);
		g_log.Message(Refactoring::LogType::Info, std::format("IDDCX Version: {:#x}", outArgs.IddCxVersion).c_str());
	}
	else {
		g_log.Message(Refactoring::LogType::Info, "Failed to get IDDCX version");
	}
	g_log.Message(Refactoring::LogType::Debug, "Testing Debug Log");
}

void InitializeD3DDeviceAndLogGPU() {
	ComPtr<ID3D11Device> d3dDevice;
	ComPtr<ID3D11DeviceContext> d3dContext;
	HRESULT hr = D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&d3dDevice,
		nullptr,
		&d3dContext);

	if (FAILED(hr)) {
		g_log.Message(Refactoring::LogType::Error, "Retrieving D3D Device GPU: Failed to create D3D11 device");
		return;
	}

	ComPtr<IDXGIDevice> dxgiDevice;
	hr = d3dDevice.As(&dxgiDevice);
	if (FAILED(hr)) {
		g_log.Message(Refactoring::LogType::Error, "Retrieving D3D Device GPU: Failed to get DXGI device");
		return;
	}

	ComPtr<IDXGIAdapter> dxgiAdapter;
	hr = dxgiDevice->GetAdapter(&dxgiAdapter);
	if (FAILED(hr)) {
		g_log.Message(Refactoring::LogType::Error, "Retrieving D3D Device GPU: Failed to get DXGI adapter");
		return;
	}

	DXGI_ADAPTER_DESC desc;
	hr = dxgiAdapter->GetDesc(&desc);
	if (FAILED(hr)) {
		g_log.Message(Refactoring::LogType::Error, "Retrieving D3D Device GPU: Failed to get GPU description");
		return;
	}

	d3dDevice.Reset();
	d3dContext.Reset();

	wstring wdesc(desc.Description);
	string utf8_desc;
	try {
		utf8_desc = Refactoring::WStringToString(wdesc);
	}
	catch (const exception& e) {
		g_log.Message(Refactoring::LogType::Error, ("Retrieving D3D Device GPU: Conversion error: " + string(e.what())).c_str());
		return;
	}

	string logtext = "Retrieving D3D Device GPU: " + utf8_desc;
	g_log.Message(Refactoring::LogType::Info, logtext.c_str());
}


// This macro creates the methods for accessing an IndirectDeviceContextWrapper as a context for a WDF object
WDF_DECLARE_CONTEXT_TYPE(IndirectDeviceContextWrapper);

extern "C" BOOL WINAPI DllMain(
	_In_ HINSTANCE hInstance,
	_In_ UINT dwReason,
	_In_opt_ LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(lpReserved);
	UNREFERENCED_PARAMETER(dwReason);

	return TRUE;
}


bool UpdateXmlSetting(std::wstring value, const wchar_t* variable) {
	const wstring settingsname = confpath + L"\\vdd_settings.xml";
	CComPtr<IStream> pFileStream;
	HRESULT hr = SHCreateStreamOnFileEx(settingsname.c_str(), STGM_READWRITE, FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, &pFileStream);
	if (FAILED(hr)) {
		g_log.Message(Refactoring::LogType::Error, "UpdatingXML: XML file could not be opened.");
		return false;
	}

	CComPtr<IXmlReader> pReader;
	hr = CreateXmlReader(__uuidof(IXmlReader), (void**)&pReader, nullptr);
	if (FAILED(hr)) {
		g_log.Message(Refactoring::LogType::Error, "UpdatingXML: Failed to create XML reader.");
		return false;
	}
	hr = pReader->SetInput(pFileStream);
	if (FAILED(hr)) {
		g_log.Message(Refactoring::LogType::Error, "UpdatingXML: Failed to set XML reader input.");
		return false;
	}

	CComPtr<IStream> pOutFileStream;
	wstring tempFileName = settingsname + L".temp";
	hr = SHCreateStreamOnFileEx(tempFileName.c_str(), STGM_CREATE | STGM_WRITE, FILE_ATTRIBUTE_NORMAL, TRUE, nullptr, &pOutFileStream);
	if (FAILED(hr)) {
		g_log.Message(Refactoring::LogType::Error, "UpdatingXML: Failed to create output file stream.");
		return false;
	}

	CComPtr<IXmlWriter> pWriter;
	hr = CreateXmlWriter(__uuidof(IXmlWriter), (void**)&pWriter, nullptr);
	if (FAILED(hr)) {
		g_log.Message(Refactoring::LogType::Error, "UpdatingXML: Failed to create XML writer.");
		return false;
	}
	hr = pWriter->SetOutput(pOutFileStream);
	if (FAILED(hr)) {
		g_log.Message(Refactoring::LogType::Error, "UpdatingXML: Failed to set XML writer output.");
		return false;
	}
	hr = pWriter->WriteStartDocument(XmlStandalone_Omit);
	if (FAILED(hr)) {
		g_log.Message(Refactoring::LogType::Error, "UpdatingXML: Failed to write start of the document.");
		return false;
	}

	XmlNodeType nodeType;
	const wchar_t* pwszLocalName;
	const wchar_t* pwszValue;
	bool variableElementFound = false;

	while (S_OK == pReader->Read(&nodeType)) {
		switch (nodeType) {
		case XmlNodeType_Element:
			pReader->GetLocalName(&pwszLocalName, nullptr);
			pWriter->WriteStartElement(nullptr, pwszLocalName, nullptr);
			break;

		case XmlNodeType_EndElement:
			pReader->GetLocalName(&pwszLocalName, nullptr);
			pWriter->WriteEndElement();
			break;

		case XmlNodeType_Text:
			pReader->GetValue(&pwszValue, nullptr);
			if (variableElementFound) {
				pWriter->WriteString(value.c_str());
				variableElementFound = false;
			}
			else {
				pWriter->WriteString(pwszValue);
			}
			break;

		case XmlNodeType_Whitespace:
			pReader->GetValue(&pwszValue, nullptr);
			pWriter->WriteWhitespace(pwszValue);
			break;

		case XmlNodeType_Comment:
			pReader->GetValue(&pwszValue, nullptr);
			pWriter->WriteComment(pwszValue);
			break;
		}

		if (nodeType == XmlNodeType_Element) {
			pReader->GetLocalName(&pwszLocalName, nullptr);
			if (pwszLocalName != nullptr && wcscmp(pwszLocalName, variable) == 0) {
				variableElementFound = true;
			}
		}
	}

	if (variableElementFound) {
		pWriter->WriteStartElement(nullptr, variable, nullptr);
		pWriter->WriteString(value.c_str());
		pWriter->WriteEndElement();
	}

	hr = pWriter->WriteEndDocument();
	if (FAILED(hr)) {
		return false;
	}

	pFileStream.Release();
	pOutFileStream.Release();
	pWriter.Release();
	pReader.Release();

	if (!MoveFileExW(tempFileName.c_str(), settingsname.c_str(), MOVEFILE_REPLACE_EXISTING)) {
		return false;
	}
	return true;
}

LUID getSetAdapterLuid() {
	AdapterOption& adapterOption = Options.Adapter;

	if (!adapterOption.hasTargetAdapter) {
		g_log.Message(Refactoring::LogType::Error,"No Gpu Found/Selected");
	}

	return adapterOption.adapterLuid;
}


void GetGpuInfo()
{
	AdapterOption& adapterOption = Options.Adapter;

	if (!adapterOption.hasTargetAdapter) {
		g_log.Message(Refactoring::LogType::Error, "No GPU found or set.");
		return;
	}

	try {
		string utf8_desc = Refactoring::WStringToString(adapterOption.target_name);
		LUID luid = getSetAdapterLuid();
		string logtext = "ASSIGNED GPU: " + utf8_desc +
			" (LUID: " + std::to_string(luid.LowPart) + "-" + std::to_string(luid.HighPart) + ")";
		g_log.Message(Refactoring::LogType::Info, logtext.c_str());
	}
	catch (const exception& e) {
		g_log.Message(Refactoring::LogType::Error, ("Error: " + string(e.what())).c_str());
	}
}

void logAvailableGPUs() {
	vector<GPUInfo> gpus;
	ComPtr<IDXGIFactory1> factory;
	if (!SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
		return;
	}
	for (UINT i = 0;; i++) {
		ComPtr<IDXGIAdapter> adapter;
		if (!SUCCEEDED(factory->EnumAdapters(i, &adapter))) {
			break;
		}
		DXGI_ADAPTER_DESC desc;
		if (!SUCCEEDED(adapter->GetDesc(&desc))) {
			continue;
		}
		GPUInfo info{ desc.Description, adapter, desc };
		gpus.push_back(info);
	}
	for (const auto& gpu : gpus) {
		auto memorysize = gpu.desc.DedicatedVideoMemory / (1024 * 1024);

		g_log.Message(Refactoring::LogType::Companion,
					  std::format("GPU Name: {} Memory: {} MB", Refactoring::WStringToString(gpu.desc.Description), memorysize).c_str());
	}
}

void ReloadDriver(HANDLE hPipe) {
	auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(hPipe);
	if (pContext && pContext->pContext) {
		pContext->pContext->InitAdapter();
		g_log.Message(Refactoring::LogType::Info, "Adapter reinitialized");
	}
}

void HandleClient(HANDLE hPipe) {
	g_pipeHandle = hPipe;
	g_log.Message(Refactoring::LogType::Pipe, "Client Handling Enabled");
	wchar_t buffer[128];
	DWORD bytesRead;
	BOOL result = ReadFile(hPipe, buffer, sizeof(buffer) - sizeof(wchar_t), &bytesRead, NULL);

	if (result && bytesRead == 0)
	{
		DisconnectNamedPipe(hPipe);
		CloseHandle(hPipe);
		g_pipeHandle = INVALID_HANDLE_VALUE;
		return;
	}

	buffer[bytesRead / sizeof(wchar_t)] = L'\0';
	auto str_buffer = Refactoring::WStringToString(buffer);
	auto pipe_tokens = Refactoring::tokenize(str_buffer, ' ');

	g_log.Message(Refactoring::LogType::Pipe, str_buffer.c_str());

	struct elements
	{
		std::string xml_key;
		//std::string comment;
		bool reload_pipe;
	};

	std::map<std::string, elements> entries;

	entries.insert({"LOGGING", {"logging.logging", false}});
	entries.insert({"LOG_DEBUG", {"logging.debuglogging", false}});
	entries.insert({"CUSTOMEDID", {"edid.CustomEdid", true}});
	entries.insert({"PREVENTSPOOF", {"edid.PreventSpoof", true}});
	entries.insert({"EdidCeaOverride", {"edid.EdidCeaOverride", true}});
	entries.insert({"HDRPLUS", {"colour.HDRPlus", true}});
	entries.insert({"SDR10", {"colour.SDR10bit", true}});
	entries.insert({"HARDWARECURSOR", {"cursor.HardwareCursor", true}});
	entries.insert({"SETGPU", {"gpu.friendlyname", true}});
	entries.insert({"SETDISPLAYCOUNT", {"monitors.count", true}});

	std::map<std::string, std::function<void (std::vector<std::string>)>> prova;

	if (pipe_tokens[0] == "PING")
	{
		g_log.SendToPipe("PONG");
		g_log.Message(Refactoring::LogType::Pipe, "Heartbeat Ping");
		return;
	}

	if (pipe_tokens[0] == "RELOAD_DRIVER")
	{
		g_log.Message(Refactoring::LogType::Companion, "Reloading the driver");
		ReloadDriver(hPipe);
	}

	auto it = entries.find(pipe_tokens[0]);
	if (it != entries.end())
	{
		if (!g_settings_manager.SetSetting(it->second.xml_key, pipe_tokens[1]))
		{
			g_log.Message(Refactoring::LogType::Companion, std::format("Set operation failed for {}", it->second.xml_key));
		}

		if (it->second.reload_pipe)
			ReloadDriver(hPipe);

		if (pipe_tokens[0] == "LOGGING")
		{
			g_log.ToggleStandardLogs(g_settings.logs.enable_standard_logs);
		}
		else if (pipe_tokens[0] == "LOG_DEBUG")
		{
			g_log.ToggleDebugLogs(g_settings.logs.enable_debug_logs);
		}
		g_log.Message(Refactoring::LogType::Companion, std::format("{} new value: {}", it->first, pipe_tokens[1]));
		return;
	}

	// D3DDEVICEGPU: LOGS, initializeD3DDeviceAndLogGPU
	if (pipe_tokens[0] == "D3DDEVICEGPU")
	{
		g_log.Message(Refactoring::LogType::Companion, "Retrieving D3D GPU (This information may be inaccurate without reloading the driver first)");
		InitializeD3DDeviceAndLogGPU();
		g_log.Message(Refactoring::LogType::Companion, "Retrieved D3D GPU");
	}
	// IDDCXVERSION: LOGS, LogIddCxVersion
	else if (pipe_tokens[0] == "IDDCXVERSION")
	{
		g_log.Message(Refactoring::LogType::Companion, "Logging iddcx version");
		LogIddCxVersion();
	}
	// GETASSIGNEDGPU: LOGS, GetGpuInfo
	else if (pipe_tokens[0] == "GETASSIGNEDGPU")
	{
		g_log.Message(Refactoring::LogType::Companion, "Retrieving Assigned GPU");
		GetGpuInfo();
		g_log.Message(Refactoring::LogType::Companion, "Retrieved Assigned GPU");
	}
	// GETALLGPUS: LOGS, logAvailableGPUs
	else if (pipe_tokens[0] == "GETALLGPUS")
	{
		g_log.Message(Refactoring::LogType::Companion, "Logging all GPUs");
		g_log.Message(Refactoring::LogType::Info,
					  "If any GPUs which shows twice but you only have one, it will most likely be the GPU the driver is attached to");
		logAvailableGPUs();
		g_log.Message(Refactoring::LogType::Companion, "Logged all GPUs");
	}
	// GETSETTINGS: recupera il valore salvato per i log, e lo manda alla pipe
	else if (pipe_tokens[0] == "GETSETTINGS")
	{
		wstring settingsResponse =
			std::format(L"SETTINGS DEBUG={:s} LOG={:s}", g_settings.logs.enable_debug_logs, g_settings.logs.enable_standard_logs);

		DWORD bytesWritten;
		DWORD bytesToWrite = static_cast<DWORD>((settingsResponse.length() + 1) * sizeof(wchar_t));
		WriteFile(hPipe, settingsResponse.c_str(), bytesToWrite, &bytesWritten, NULL);
	}
	else
	{
		g_log.Message(Refactoring::LogType::Warning, "Command not recognized.");
	}

	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);
	g_pipeHandle = INVALID_HANDLE_VALUE; // This value determines whether or not all data gets sent back through the pipe or just the handling pipe data
}


DWORD WINAPI NamedPipeServer(LPVOID lpParam) {
	UNREFERENCED_PARAMETER(lpParam);

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = FALSE;
	const wchar_t* sddl = L"D:(A;;GA;;;WD)";
	g_log.Message(Refactoring::LogType::Debug, "Starting pipe with parameters: D:(A;;GA;;;WD)");
	if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
		sddl, SDDL_REVISION_1, &sa.lpSecurityDescriptor, NULL)) {
		g_log.Message(Refactoring::LogType::Error, std::format("[Named Pipe Server] Error Converting security descriptor to wide-string. Error code: {}", GetLastError()).c_str());
		return 1;
	}
	HANDLE hPipe;
	while (g_Running) {
		hPipe = CreateNamedPipeW(
			PIPE_NAME,
			PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES,
			512, 512,
			0,
			&sa);

		if (hPipe == INVALID_HANDLE_VALUE) {
			g_log.Message(Refactoring::LogType::Error, std::format("[Named Pipe Server] Pipe handle invalid. Error code: {}", GetLastError()).c_str());
			LocalFree(sa.lpSecurityDescriptor);
			return 1;
		}

		BOOL connected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
		if (connected) {
			g_log.Message(Refactoring::LogType::Pipe, "Client Connected");
			HandleClient(hPipe);
		}
		else {
			CloseHandle(hPipe);
		}
	}
	LocalFree(sa.lpSecurityDescriptor);
	return 0;
}

void StartNamedPipeServer() {
	g_log.Message(Refactoring::LogType::Pipe, "Starting Pipe");
	hPipeThread = CreateThread(NULL, 0, NamedPipeServer, NULL, 0, NULL);
	if (hPipeThread == NULL) {
		g_log.Message(Refactoring::LogType::Error, std::format("Pipe was not created. Error code: {}", GetLastError()).c_str());
	}
	else {
		g_log.Message(Refactoring::LogType::Pipe, "Pipe created");
	}
}

void StopNamedPipeServer() {
	g_log.Message(Refactoring::LogType::Pipe, "Stopping Pipe");
	{
		lock_guard<mutex> lock(g_Mutex);
		g_Running = false;
	}
	if (hPipeThread) {
		HANDLE hPipe = CreateFileW(
			PIPE_NAME,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);

		if (hPipe != INVALID_HANDLE_VALUE) {
			DisconnectNamedPipe(hPipe);
			CloseHandle(hPipe);
		}

		WaitForSingleObject(hPipeThread, INFINITE);
		CloseHandle(hPipeThread);
		hPipeThread = NULL;
		g_log.Message(Refactoring::LogType::Pipe, "Stopped Pipe");
	}
}

extern "C" EVT_WDF_DRIVER_UNLOAD EvtDriverUnload;

VOID
EvtDriverUnload(
	_In_ WDFDRIVER Driver
)
{
	UNREFERENCED_PARAMETER(Driver);
	StopNamedPipeServer();
	g_log.Message(Refactoring::LogType::Info, "Driver Unloaded");
}

_Use_decl_annotations_
extern "C" NTSTATUS DriverEntry(
	PDRIVER_OBJECT  pDriverObject,
	PUNICODE_STRING pRegistryPath
)
{
	WDF_DRIVER_CONFIG Config;
	NTSTATUS Status;

	WDF_OBJECT_ATTRIBUTES Attributes;
	WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);

	WDF_DRIVER_CONFIG_INIT(&Config, VirtualDisplayDriverDeviceAdd);

	Config.EvtDriverUnload = EvtDriverUnload;

	g_settings_manager.Init();
	g_settings_manager.LoadSettings();

	g_colours_iddcx.HDR_COLOR = g_settings.colours.hdr_plus ? IDDCX_BITS_PER_COMPONENT_12 : IDDCX_BITS_PER_COMPONENT_10;
	g_colours_iddcx.SDR_COLOR = g_settings.colours.sdr10 ? IDDCX_BITS_PER_COMPONENT_10 : IDDCX_BITS_PER_COMPONENT_8;

	if (g_settings.cursor.xor_cursor_support_level < 0 || g_settings.cursor.xor_cursor_support_level > 3)
	{
		g_log.Message(Refactoring::LogType::Warning, "Selected Xor Level unsupported, defaulting to IDDCX_XOR_CURSOR_SUPPORT_FULL");
		g_cursor_iddcx.xor_cursor_support_level = IDDCX_XOR_CURSOR_SUPPORT_FULL;
	}
	else {
		g_cursor_iddcx.xor_cursor_support_level =
			static_cast<IDDCX_XOR_CURSOR_SUPPORT>(g_settings.cursor.xor_cursor_support_level);
	}

	std::string xorCursorSupportLevelName = XorCursorSupportLevelToString(g_cursor_iddcx.xor_cursor_support_level);

	g_log.Message(Refactoring::LogType::Info, ("Selected Xor Cursor Support Level: " + xorCursorSupportLevelName).c_str());

	g_log.Message(Refactoring::LogType::Info, "Driver Starting");
	g_log.Message(Refactoring::LogType::Info, Refactoring::WStringToString(confpath).c_str());
	LogIddCxVersion();

	Status = WdfDriverCreate(pDriverObject, pRegistryPath, &Attributes, &Config, WDF_NO_HANDLE);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}

	StartNamedPipeServer();

	return Status;
}

vector<string> split(string& input, char delimiter)
{
	istringstream stream(input);
	string field;
	vector<string> result;
	while (getline(stream, field, delimiter)) {
		result.push_back(field);
	}
	return result;
}


void loadSettings() {
	const wstring settingsname = confpath + L"\\vdd_settings.xml";
	const wstring& filename = settingsname;
	if (PathFileExistsW(filename.c_str())) {
		CComPtr<IStream> pStream;
		CComPtr<IXmlReader> pReader;
		HRESULT hr = SHCreateStreamOnFileW(filename.c_str(), STGM_READ, &pStream);
		if (FAILED(hr)) {
			g_log.Message(Refactoring::LogType::Error, "Loading Settings: Failed to create file stream.");
			return; 
		}
		hr = CreateXmlReader(__uuidof(IXmlReader), (void**)&pReader, NULL);
		if (FAILED(hr)) {
			g_log.Message(Refactoring::LogType::Error, "Loading Settings: Failed to create XmlReader.");
			return;
		}
		hr = pReader->SetInput(pStream);
		if (FAILED(hr)) {
			g_log.Message(Refactoring::LogType::Error, "Loading Settings: Failed to set input stream.");
			return;
		}

		XmlNodeType nodeType;
		const WCHAR* pwszLocalName;
		const WCHAR* pwszValue;
		UINT cwchLocalName;
		UINT cwchValue;
		wstring currentElement;
		wstring width, height, refreshRate;
		vector<Refactoring::Resolution> res;
		wstring gpuFriendlyName;
		UINT monitorcount = 1;
		set<tuple<int, int>> resolutions;
		vector<int> globalRefreshRates;

		while (S_OK == (hr = pReader->Read(&nodeType))) {
			switch (nodeType) {
			case XmlNodeType_Element:
				hr = pReader->GetLocalName(&pwszLocalName, &cwchLocalName);
				if (FAILED(hr)) {
					return;
				}
				currentElement = wstring(pwszLocalName, cwchLocalName);
				break;
			case XmlNodeType_Text:
				hr = pReader->GetValue(&pwszValue, &cwchValue);
				if (FAILED(hr)) {
					return;
				}
				if (currentElement == L"count") {
					monitorcount = stoi(wstring(pwszValue, cwchValue));
					if (monitorcount == 0) {
						monitorcount = 1;
						g_log.Message(Refactoring::LogType::Info, "Loading singular monitor (Monitor Count is not valid)");
					}
				}
				else if (currentElement == L"friendlyname") {
					gpuFriendlyName = wstring(pwszValue, cwchValue);
				}
				else if (currentElement == L"width") {
					width = wstring(pwszValue, cwchValue);
					if (width.empty()) {
						width = L"800";
					}
				}
				else if (currentElement == L"height") {
					height = wstring(pwszValue, cwchValue);
					if (height.empty()) {
						height = L"600";
					}
					resolutions.insert(make_tuple(stoi(width), stoi(height)));
				}
				else if (currentElement == L"refresh_rate") {
					refreshRate = wstring(pwszValue, cwchValue);
					if (refreshRate.empty()) {
						refreshRate = L"30";
					}
					int vsync_num, vsync_den;
					float_to_vsync(stof(refreshRate), vsync_num, vsync_den);

					res.push_back(Refactoring::Resolution(stoi(width), stoi(height), vsync_num, vsync_den));
					stringstream ss;
					ss << "Added: " << stoi(width) << "x" << stoi(height) << " @ " << vsync_num << "/" << vsync_den << "Hz";
					g_log.Message(Refactoring::LogType::Debug, ss.str().c_str());
				}
				else if (currentElement == L"g_refresh_rate") {
					globalRefreshRates.push_back(stoi(wstring(pwszValue, cwchValue)));
				}
				break;
			}
		}

		for (int globalRate : globalRefreshRates) {
			for (const auto& resTuple : resolutions) {
				int global_width = get<0>(resTuple);
				int global_height = get<1>(resTuple);

				int vsync_num, vsync_den;
				float_to_vsync(static_cast<float>(globalRate), vsync_num, vsync_den);
				res.push_back(Refactoring::Resolution(global_width, global_height, vsync_num, vsync_den));
			}
		}

		numVirtualDisplays = monitorcount;
		gpuname = gpuFriendlyName;
		monitorModes = res;
		RebuildKnownMonitorModesCache();
		
		g_log.Message(Refactoring::LogType::Info,"Using vdd_settings.xml");
		return;
	}
	const wstring optionsname = confpath + L"\\option.txt";
	ifstream ifs(optionsname);
	if (ifs.is_open()) {
    string line;
		if (getline(ifs, line) && !line.empty())
		{
			numVirtualDisplays = stoi(line);
			vector<Refactoring::Resolution> res;

			while (getline(ifs, line))
			{
				vector<string> strvec = split(line, ',');
				if (strvec.size() == 3 && strvec[0].substr(0, 1) != "#")
				{
					int vsync_num, vsync_den;
					float_to_vsync(stof(strvec[2]), vsync_num, vsync_den);
					res.push_back({stoi(strvec[0]), stoi(strvec[1]), vsync_num, vsync_den});
				}
			}

			g_log.Message(Refactoring::LogType::Info, "Using option.txt");
			monitorModes = res;
			RebuildKnownMonitorModesCache();
			for (const auto &mode : res)
			{
				g_log.Message(Refactoring::LogType::Debug,
							  std::format("Resolution: {}x{} @ {}/{} Hz", mode.width, mode.height, mode.refresh_num, mode.refresh_den).c_str());
			}
			return;
		}
		else
		{
			g_log.Message(Refactoring::LogType::Warning, "option.txt is empty or the first line is invalid. Enabling Fallback");
		}
	}


	numVirtualDisplays = 1;
	vector<Refactoring::Resolution> res;
	vector<tuple<int, int, float>> fallbackRes = {
		{800, 600, 30.0f},
		{800, 600, 60.0f},
		{800, 600, 90.0f},
		{800, 600, 120.0f},
		{800, 600, 144.0f},
		{800, 600, 165.0f},
		{1280, 720, 30.0f},
		{1280, 720, 60.0f},
		{1280, 720, 90.0f},
		{1280, 720, 130.0f},
		{1280, 720, 144.0f},
		{1280, 720, 165.0f},
		{1366, 768, 30.0f},
		{1366, 768, 60.0f},
		{1366, 768, 90.0f},
		{1366, 768, 120.0f},
		{1366, 768, 144.0f},
		{1366, 768, 165.0f},
		{1920, 1080, 30.0f},
		{1920, 1080, 60.0f},
		{1920, 1080, 90.0f},
		{1920, 1080, 120.0f},
		{1920, 1080, 144.0f},
		{1920, 1080, 165.0f},
		{2560, 1440, 30.0f},
		{2560, 1440, 60.0f},
		{2560, 1440, 90.0f},
		{2560, 1440, 120.0f},
		{2560, 1440, 144.0f},
		{2560, 1440, 165.0f},
		{3840, 2160, 30.0f},
		{3840, 2160, 60.0f},
		{3840, 2160, 90.0f},
		{3840, 2160, 120.0f},
		{3840, 2160, 144.0f},
		{3840, 2160, 165.0f}
	};

	g_log.Message(Refactoring::LogType::Info, "Loading Fallback - no settings found");

	for (const auto& mode : fallbackRes) {
		int width, height;
		float refreshRate;
		tie(width, height, refreshRate) = mode;

		int vsync_num, vsync_den;
		float_to_vsync(refreshRate, vsync_num, vsync_den);

		res.push_back(Refactoring::Resolution(width, height, vsync_num, vsync_den));

		g_log.Message(Refactoring::LogType::Debug, std::format("Resolution: {}x{} @ {}/{} Hz", width, height, vsync_num, vsync_den).c_str());
	}

	monitorModes = res;
	RebuildKnownMonitorModesCache();
	return;

}

_Use_decl_annotations_
NTSTATUS VirtualDisplayDriverDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT pDeviceInit)
{
	NTSTATUS Status = STATUS_SUCCESS;
	WDF_PNPPOWER_EVENT_CALLBACKS PnpPowerCallbacks;

	UNREFERENCED_PARAMETER(Driver);

	//logStream << "Initializing device:"
	//	<< "\n  DeviceInit Pointer: " << static_cast<void*>(pDeviceInit);
	g_log.Message(Refactoring::LogType::Debug, std::format("Initializing device:\n  DeviceInit Pointer: {}", static_cast<void *>(pDeviceInit)).c_str());

	// Register for power callbacks - in this sample only power-on is needed
	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpPowerCallbacks);
	PnpPowerCallbacks.EvtDeviceD0Entry = VirtualDisplayDriverDeviceD0Entry;
	WdfDeviceInitSetPnpPowerEventCallbacks(pDeviceInit, &PnpPowerCallbacks);

	IDD_CX_CLIENT_CONFIG IddConfig;
	IDD_CX_CLIENT_CONFIG_INIT(&IddConfig);

	stringstream logStream;
	logStream.str("");
	logStream << "Configuring IDD_CX client:"
		<< "\n  EvtIddCxAdapterInitFinished: " << (IddConfig.EvtIddCxAdapterInitFinished ? "Set" : "Not Set")
		<< "\n  EvtIddCxMonitorGetDefaultDescriptionModes: " << (IddConfig.EvtIddCxMonitorGetDefaultDescriptionModes ? "Set" : "Not Set")
		<< "\n  EvtIddCxMonitorAssignSwapChain: " << (IddConfig.EvtIddCxMonitorAssignSwapChain ? "Set" : "Not Set")
		<< "\n  EvtIddCxMonitorUnassignSwapChain: " << (IddConfig.EvtIddCxMonitorUnassignSwapChain ? "Set" : "Not Set");
	g_log.Message(Refactoring::LogType::Debug, logStream.str().c_str());

	// If the driver wishes to handle custom IoDeviceControl requests, it's necessary to use this callback since IddCx
	// redirects IoDeviceControl requests to an internal queue. This sample does not need this.
	// IddConfig.EvtIddCxDeviceIoControl = VirtualDisplayDriverIoDeviceControl;

	loadSettings();

	if (gpuname.empty() || gpuname == L"default") {
		const wstring adaptername = confpath + L"\\adapter.txt";
		Options.Adapter.load(adaptername.c_str());
		g_log.Message(Refactoring::LogType::Info, "Attempting to Load GPU from adapter.txt");
	}
	else {
		Options.Adapter.xmlprovide(gpuname);
		g_log.Message(Refactoring::LogType::Info, "Loading GPU from vdd_settings.xml");
	}

	GetGpuInfo();

	IddConfig.EvtIddCxAdapterInitFinished = VirtualDisplayDriverAdapterInitFinished;

	IddConfig.EvtIddCxMonitorGetDefaultDescriptionModes = VirtualDisplayDriverMonitorGetDefaultModes;
	IddConfig.EvtIddCxMonitorAssignSwapChain = VirtualDisplayDriverMonitorAssignSwapChain;
	IddConfig.EvtIddCxMonitorUnassignSwapChain = VirtualDisplayDriverMonitorUnassignSwapChain;

	if (IDD_IS_FIELD_AVAILABLE(IDD_CX_CLIENT_CONFIG, EvtIddCxAdapterQueryTargetInfo))
	{
		IddConfig.EvtIddCxAdapterQueryTargetInfo = VirtualDisplayDriverEvtIddCxAdapterQueryTargetInfo;
		IddConfig.EvtIddCxMonitorSetDefaultHdrMetaData = VirtualDisplayDriverEvtIddCxMonitorSetDefaultHdrMetadata;
		IddConfig.EvtIddCxParseMonitorDescription2 = VirtualDisplayDriverEvtIddCxParseMonitorDescription2;
		IddConfig.EvtIddCxMonitorQueryTargetModes2 = VirtualDisplayDriverEvtIddCxMonitorQueryTargetModes2;
		IddConfig.EvtIddCxAdapterCommitModes2 = VirtualDisplayDriverEvtIddCxAdapterCommitModes2;
		IddConfig.EvtIddCxMonitorSetGammaRamp = VirtualDisplayDriverEvtIddCxMonitorSetGammaRamp;
	}
	else {
		IddConfig.EvtIddCxParseMonitorDescription = VirtualDisplayDriverParseMonitorDescription;
		IddConfig.EvtIddCxMonitorQueryTargetModes = VirtualDisplayDriverMonitorQueryModes;
		IddConfig.EvtIddCxAdapterCommitModes = VirtualDisplayDriverAdapterCommitModes;
	}

	Status = IddCxDeviceInitConfig(pDeviceInit, &IddConfig);
	if (!NT_SUCCESS(Status))
	{
		g_log.Message(Refactoring::LogType::Error, std::format("IddCxDeviceInitConfig failed with status: {}", Status).c_str());
		return Status;
	}

	WDF_OBJECT_ATTRIBUTES Attr;
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attr, IndirectDeviceContextWrapper);
	Attr.EvtCleanupCallback = [](WDFOBJECT Object)
		{
			// Automatically cleanup the context when the WDF object is about to be deleted
			auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(Object);
			if (pContext)
			{
				pContext->Cleanup();
			}
		};

	g_log.Message(Refactoring::LogType::Debug, "Creating device with WdfDeviceCreate:");

	WDFDEVICE Device = nullptr;
	Status = WdfDeviceCreate(&pDeviceInit, &Attr, &Device);
	if (!NT_SUCCESS(Status))
	{
		g_log.Message(Refactoring::LogType::Error, std::format("WdfDeviceCreate failed with status: {}", Status).c_str());
		return Status;
	}

	Status = IddCxDeviceInitialize(Device);
	if (!NT_SUCCESS(Status))
	{
		g_log.Message(Refactoring::LogType::Error, std::format("IddCxDeviceInitialize failed with status: {}", Status).c_str());
		return Status;
	}

	// Create a new device context object and attach it to the WDF device object
	/*
	auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(Device);
	pContext->pContext = new IndirectDeviceContext(Device);
	*/ // code to return uncase the device context wrapper isnt found (Most likely insufficient resources)

	auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(Device);
	if (pContext)
	{
		pContext->pContext = new IndirectDeviceContext(Device);
		g_log.Message(Refactoring::LogType::Debug, "Device context initialized and attached to WDF device.");
	}
	else
	{
		g_log.Message(Refactoring::LogType::Error, "Failed to get device context wrapper.");
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	return Status;
}

_Use_decl_annotations_
NTSTATUS VirtualDisplayDriverDeviceD0Entry(WDFDEVICE Device, WDF_POWER_DEVICE_STATE PreviousState)
{
	//UNREFERENCED_PARAMETER(PreviousState);

	stringstream logStream;

	// Log the entry into D0 state
	logStream << "Entering D0 power state:"
		<< "\n  Device Handle: " << static_cast<void*>(Device)
		<< "\n  Previous State: " << PreviousState;
	g_log.Message(Refactoring::LogType::Debug, logStream.str().c_str());

	// This function is called by WDF to start the device in the fully-on power state.

	/*
	auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(Device);
	pContext->pContext->InitAdapter();
	*/ //Added error handling incase fails to get device context

	auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(Device);
	if (pContext && pContext->pContext)
	{
		pContext->pContext->InitAdapter();
		g_log.Message(Refactoring::LogType::Debug, "InitAdapter called successfully.");
	}
	else
	{
		g_log.Message(Refactoring::LogType::Error, "Failed to get device context.");
		return STATUS_INSUFFICIENT_RESOURCES;
	}


	return STATUS_SUCCESS;
}

#pragma region Direct3DDevice

Direct3DDevice::Direct3DDevice(LUID AdapterLuid) : AdapterLuid(AdapterLuid)
{
}

Direct3DDevice::Direct3DDevice() : AdapterLuid({})
{
}

HRESULT Direct3DDevice::Init()
{
	HRESULT hr;
	// The DXGI factory could be cached, but if a new render adapter appears on the system, a new factory needs to be
	// created. If caching is desired, check DxgiFactory->IsCurrent() each time and recreate the factory if !IsCurrent.

	g_log.Message(Refactoring::LogType::Debug, "Initializing Direct3DDevice...");

	hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&DxgiFactory));
	if (FAILED(hr))
	{
		g_log.Message(Refactoring::LogType::Error, std::format("Failed to create DXGI factory. HRESULT: {}", hr).c_str());
		return hr;
	}
	g_log.Message(Refactoring::LogType::Debug, "DXGI factory created successfully.");

	// Find the specified render adapter
	hr = DxgiFactory->EnumAdapterByLuid(AdapterLuid, IID_PPV_ARGS(&Adapter));
	if (FAILED(hr))
	{
		g_log.Message(Refactoring::LogType::Error, std::format("Failed to enumerate adapter by LUID. HRESULT: {}", hr).c_str());
		return hr;
	}

	DXGI_ADAPTER_DESC desc;
	Adapter->GetDesc(&desc);
	g_log.Message(Refactoring::LogType::Info, std::format("Adapter found: {} (Vendor ID: {}, Device ID: {})", Refactoring::WStringToString(desc.Description), desc.VendorId, desc.DeviceId).c_str());


#if 0 // Test code
	{
		FILE* file;
		fopen_s(&file, "C:\\VirtualDisplayDriver\\desc_hdr.bin", "wb");

		DXGI_ADAPTER_DESC desc;
		Adapter->GetDesc(&desc);

		fwrite(&desc, 1, sizeof(desc), file);
		fclose(file);
	}
#endif

	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
	D3D_FEATURE_LEVEL featureLevel;

	// Create a D3D device using the render adapter. BGRA support is required by the WHQL test suite.
	hr = D3D11CreateDevice(Adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &Device, &featureLevel, &DeviceContext);
	if (FAILED(hr))
	{
		// If creating the D3D device failed, it's possible the render GPU was lost (e.g. detachable GPU) or else the
		// system is in a transient state.
		g_log.Message(Refactoring::LogType::Error, std::format("Failed to create Direct3D device. HRESULT: {}", hr).c_str());
		g_log.Message(Refactoring::LogType::Error, std::format("If creating the D3D device failed, it's possible the render GPU was lost (e.g. detachable GPU) or else the system "
								"is in a transient state. {}", hr).c_str());
		return hr;
	}
	g_log.Message(Refactoring::LogType::Info, std::format("Direct3D device created successfully. Feature Level: {:#x}", static_cast<int>(featureLevel)).c_str());

	return S_OK;
}

#pragma endregion

#pragma region SwapChainProcessor

SwapChainProcessor::SwapChainProcessor(IDDCX_SWAPCHAIN hSwapChain, shared_ptr<Direct3DDevice> Device, HANDLE NewFrameEvent)
	: m_hSwapChain(hSwapChain), m_Device(Device), m_hAvailableBufferEvent(NewFrameEvent)
{
	stringstream logStream;

	logStream << "Constructing SwapChainProcessor:"
		<< "\n  SwapChain Handle: " << static_cast<void*>(hSwapChain)
		<< "\n  Device Pointer: " << static_cast<void*>(Device.get())
		<< "\n  NewFrameEvent Handle: " << NewFrameEvent;
	g_log.Message(Refactoring::LogType::Debug, logStream.str().c_str());

	m_hTerminateEvent.Attach(CreateEvent(nullptr, FALSE, FALSE, nullptr));
	if (!m_hTerminateEvent.Get())
	{
		g_log.Message(Refactoring::LogType::Error, std::format("Failed to create terminate event. GetLastError: {}", GetLastError()).c_str());
	}
	else
	{
		g_log.Message(Refactoring::LogType::Debug, "Terminate event created successfully.");
	}

	// Immediately create and run the swap-chain processing thread, passing 'this' as the thread parameter
	m_hThread.Attach(CreateThread(nullptr, 0, RunThread, this, 0, nullptr));
	if (!m_hThread.Get())
	{
		g_log.Message(Refactoring::LogType::Error, std::format("Failed to create swap-chain processing thread. GetLastError: {}", GetLastError()).c_str());
	}
	else
	{
		g_log.Message(Refactoring::LogType::Debug, "Swap-chain processing thread created and started successfully.");
	}
}

SwapChainProcessor::~SwapChainProcessor()
{
	stringstream logStream;

	logStream << "Destructing SwapChainProcessor:";

	g_log.Message(Refactoring::LogType::Debug, logStream.str().c_str());
	// Alert the swap-chain processing thread to terminate
	//SetEvent(m_hTerminateEvent.Get()); changed for error handling + log purposes 

	if (SetEvent(m_hTerminateEvent.Get()))
	{
		g_log.Message(Refactoring::LogType::Debug, "Terminate event signaled successfully.");
	}
	else
	{
		g_log.Message(Refactoring::LogType::Error, std::format("Failed to signal terminate event. GetLastError: {}", GetLastError()).c_str());
	}

	if (m_hThread.Get())
	{
		// Wait for the thread to terminate
		DWORD waitResult = WaitForSingleObject(m_hThread.Get(), INFINITE);
		switch (waitResult)
		{
		case WAIT_OBJECT_0:
			g_log.Message(Refactoring::LogType::Debug, "Thread terminated successfully.");
			break;
		case WAIT_ABANDONED:
			g_log.Message(Refactoring::LogType::Error, std::format("Thread wait was abandoned. GetLastError: {}", GetLastError()).c_str());
			break;
		case WAIT_TIMEOUT:
			g_log.Message(Refactoring::LogType::Error, std::format("Thread wait timed out. This should not happen. GetLastError: {}", GetLastError()).c_str());
			break;
		default:
			g_log.Message(Refactoring::LogType::Error, std::format("Unexpected result from WaitForSingleObject. GetLastError: {}", GetLastError()).c_str());
			break;
		}
	}
	else
	{
		g_log.Message(Refactoring::LogType::Error, "No valid thread handle to wait for.");
	}
}

DWORD CALLBACK SwapChainProcessor::RunThread(LPVOID Argument)
{
	g_log.Message(Refactoring::LogType::Debug, std::format("RunThread started. Argument: {}", Argument).c_str());
	reinterpret_cast<SwapChainProcessor*>(Argument)->Run();
	return 0;
}

void SwapChainProcessor::Run()
{
	g_log.Message(Refactoring::LogType::Debug, "Run method started.");

	// For improved performance, make use of the Multimedia Class Scheduler Service, which will intelligently
	// prioritize this thread for improved throughput in high CPU-load scenarios.
	DWORD AvTask = 0;
	HANDLE AvTaskHandle = AvSetMmThreadCharacteristicsW(L"Distribution", &AvTask);

	if (AvTaskHandle)
	{
		g_log.Message(Refactoring::LogType::Debug, std::format("Multimedia thread characteristics set successfully. AvTask: {}", AvTask).c_str());
	}
	else
	{
		g_log.Message(Refactoring::LogType::Error, std::format("Failed to set multimedia thread characteristics. GetLastError: {}", GetLastError()).c_str());
	}

	RunCore();

	g_log.Message(Refactoring::LogType::Debug, "Core processing function RunCore() completed.");

	// Always delete the swap-chain object when swap-chain processing loop terminates in order to kick the system to
	// provide a new swap-chain if necessary.
	/*
	WdfObjectDelete((WDFOBJECT)m_hSwapChain);
	m_hSwapChain = nullptr;   added error handling in so its not called to delete swap chain if its not needed.
	*/
	if (m_hSwapChain)
	{
		WdfObjectDelete((WDFOBJECT)m_hSwapChain);
		g_log.Message(Refactoring::LogType::Debug, "Swap-chain object deleted.");
		m_hSwapChain = nullptr;
	}
	else
	{
		g_log.Message(Refactoring::LogType::Warning, "No valid swap-chain object to delete.");
	}
	/*
	AvRevertMmThreadCharacteristics(AvTaskHandle);
	*/ //error handling when reversing multimedia thread characteristics 
	if (AvRevertMmThreadCharacteristics(AvTaskHandle))
	{
		g_log.Message(Refactoring::LogType::Debug, "Multimedia thread characteristics reverted successfully.");
	}
	else
	{
		g_log.Message(Refactoring::LogType::Error, std::format("Failed to revert multimedia thread characteristics. GetLastError: {}", GetLastError()).c_str());
	}
}

void SwapChainProcessor::RunCore()
{
	stringstream logStream;
	DWORD retryDelay = 1;
	const DWORD maxRetryDelay = 100;
	int retryCount = 0;
	const int maxRetries = 5;

	// Get the DXGI device interface
	ComPtr<IDXGIDevice> DxgiDevice;
	HRESULT hr = m_Device->Device.As(&DxgiDevice);
	if (FAILED(hr))
	{
		g_log.Message(Refactoring::LogType::Error, std::format("Failed to get DXGI device interface. HRESULT: {}", hr).c_str());
		return;
	}
	g_log.Message(Refactoring::LogType::Info, "DXGI device interface obtained successfully.");
	//g_log.Message(Refactoring::LogType::Debug, logStream.str().c_str());


	// Validate that our device is still valid before setting it
	if (!m_Device || !m_Device->Device) {
		g_log.Message(Refactoring::LogType::Error, "Direct3DDevice became invalid during SwapChain processing");
		return;
	}

	IDARG_IN_SWAPCHAINSETDEVICE SetDevice = {};
	SetDevice.pDevice = DxgiDevice.Get();

	hr = IddCxSwapChainSetDevice(m_hSwapChain, &SetDevice);
	if (FAILED(hr))
	{
		g_log.Message(Refactoring::LogType::Error, std::format("Failed to set device to swap chain. HRESULT: {}", hr).c_str());
		return;
	}
	g_log.Message(Refactoring::LogType::Debug, "Device set to swap chain successfully.");
	g_log.Message(Refactoring::LogType::Debug, "Starting buffer acquisition and release loop.");

	// Acquire and release buffers in a loop
	for (;;)
	{
		ComPtr<IDXGIResource> AcquiredBuffer;

		// Ask for the next buffer from the producer
		IDARG_IN_RELEASEANDACQUIREBUFFER2 BufferInArgs = {};
		BufferInArgs.Size = sizeof(BufferInArgs);
		IDXGIResource* pSurface;

		if (IDD_IS_FUNCTION_AVAILABLE(IddCxSwapChainReleaseAndAcquireBuffer2)) {
			IDARG_OUT_RELEASEANDACQUIREBUFFER2 Buffer = {};
			hr = IddCxSwapChainReleaseAndAcquireBuffer2(m_hSwapChain, &BufferInArgs, &Buffer);
			pSurface = Buffer.MetaData.pSurface;
		}
		else
		{
			IDARG_OUT_RELEASEANDACQUIREBUFFER Buffer = {};
			hr = IddCxSwapChainReleaseAndAcquireBuffer(m_hSwapChain, &Buffer);
			pSurface = Buffer.MetaData.pSurface;
		}
		// AcquireBuffer immediately returns STATUS_PENDING if no buffer is yet available
		logStream.str("");
		if (hr == E_PENDING)
		{
			HANDLE waitHandles[2] = {};
			DWORD waitHandleCount = 0;

			if (m_hAvailableBufferEvent != nullptr && m_hAvailableBufferEvent != INVALID_HANDLE_VALUE)
			{
				waitHandles[waitHandleCount++] = m_hAvailableBufferEvent;
			}

			if (m_hTerminateEvent.Get())
			{
				waitHandles[waitHandleCount++] = m_hTerminateEvent.Get();
			}

			if (waitHandleCount == 0)
			{
				g_log.Message(Refactoring::LogType::Error, "No valid wait handles available while waiting for the next frame.");
				break;
			}

			DWORD WaitResult = WaitForMultipleObjects(waitHandleCount, waitHandles, FALSE, INFINITE);

			logStream << "Buffer acquisition pending. WaitResult: " << WaitResult;

			if (WaitResult == WAIT_OBJECT_0)
			{
				continue;
			}
			else if (waitHandleCount > 1 && WaitResult == WAIT_OBJECT_0 + 1)
			{
				logStream << "Terminate event signaled. Exiting loop.";
				break;
			}
			else if (waitHandleCount == 1 && waitHandles[0] == m_hTerminateEvent.Get() && WaitResult == WAIT_OBJECT_0)
			{
				logStream << "Terminate event signaled. Exiting loop.";
				break;
			}
			else
			{
				hr = HRESULT_FROM_WIN32(WaitResult == WAIT_FAILED ? GetLastError() : WaitResult);
				logStream << "Unexpected wait result. HRESULT: " << hr;
				g_log.Message(Refactoring::LogType::Error, logStream.str().c_str());
				break;
			}
		}
		else if (SUCCEEDED(hr))
		{
			// Reset retry delay and count on successful buffer acquisition
			retryDelay = 1;
			retryCount = 0;
			
			AcquiredBuffer.Attach(pSurface);

			// ==============================
			// TODO: Process the frame here
			//
			// This is the most performance-critical section of code in an IddCx driver. It's important that whatever
			// is done with the acquired surface be finished as quickly as possible. This operation could be:
			//  * a GPU copy to another buffer surface for later processing (such as a staging surface for mapping to CPU memory)
			//  * a GPU encode operation
			//  * a GPU VPBlt to another surface
			//  * a GPU custom compute shader encode operation
			// ==============================

			AcquiredBuffer.Reset();
			//g_log.Message(Refactoring::LogType::Debug, "Reset buffer");
			hr = IddCxSwapChainFinishedProcessingFrame(m_hSwapChain);
			if (FAILED(hr))
			{
				break;
			}

			// ==============================
			// TODO: Report frame statistics once the asynchronous encode/send work is completed
			//
			// Drivers should report information about sub-frame timings, like encode time, send time, etc.
			// ==============================
			// IddCxSwapChainReportFrameStatistics(m_hSwapChain, ...);
		}
		else
		{
			//logStream.str(""); // Clear the stream
			if (hr == DXGI_ERROR_ACCESS_LOST && retryCount < maxRetries)
			{
				g_log.Message(
					Refactoring::LogType::Warning,
					std::format("DXGI_ERROR_ACCESS_LOST detected. Retry {}/{} after {} ms delay.", (retryCount + 1), maxRetries, retryDelay).c_str());
				//logStream << "DXGI_ERROR_ACCESS_LOST detected. Retry " << (retryCount + 1) << "/" << maxRetries << " after " << retryDelay << "ms delay.";
				//g_log.Message(Refactoring::LogType::Warning, logStream.str().c_str());
				Sleep(retryDelay);
				retryDelay = min(retryDelay * 2, maxRetryDelay);
				retryCount++;
				continue;
			}
			else
			{
				if (hr == DXGI_ERROR_ACCESS_LOST)
				{
					logStream << "DXGI_ERROR_ACCESS_LOST: Maximum retries (" << maxRetries << ") reached. Exiting loop.";
				}
				else
				{
					logStream << "Failed to acquire buffer. Exiting loop. HRESULT: " << hr;
				}
				g_log.Message(Refactoring::LogType::Error, logStream.str().c_str());
				// The swap-chain was likely abandoned, so exit the processing loop
				break;
			}
		}
	}
}

#pragma endregion

#pragma region IndirectDeviceContext

const UINT64 MHZ = 1000000;
const UINT64 KHZ = 1000;

constexpr DISPLAYCONFIG_VIDEO_SIGNAL_INFO dispinfo(UINT32 h, UINT32 v, UINT32 rn, UINT32 rd) {
	const UINT32 clock_rate = rn * (v + 4) * (v + 4) / rd + 1000;
	return {
	  clock_rate,                                      // pixel clock rate [Hz]
	{ clock_rate, v + 4 },                         // fractional horizontal refresh rate [Hz]
	{ clock_rate, (v + 4) * (v + 4) },          // fractional vertical refresh rate [Hz]
	{ h, v },                                    // (horizontal, vertical) active pixel resolution
	{ h + 4, v + 4 },                         // (horizontal, vertical) total pixel resolution
	{ { 255, 0 }},                                   // video standard and vsync divider
	DISPLAYCONFIG_SCANLINE_ORDERING_PROGRESSIVE
	};
}

vector<BYTE> hardcodedEdid =
{
0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x36, 0x94, 0x37, 0x13, 0xe7, 0x1e, 0xe7, 0x1e,
0x1c, 0x22, 0x01, 0x03, 0x80, 0x32, 0x1f, 0x78, 0x07, 0xee, 0x95, 0xa3, 0x54, 0x4c, 0x99, 0x26,
0x0f, 0x50, 0x54, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x3a, 0x80, 0x18, 0x71, 0x38, 0x2d, 0x40, 0x58, 0x2c,
0x45, 0x00, 0x63, 0xc8, 0x10, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x17, 0xf0, 0x0f,
0xff, 0x37, 0x00, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc,
0x00, 0x56, 0x44, 0x44, 0x20, 0x62, 0x79, 0x20, 0x4d, 0x54, 0x54, 0x0a, 0x20, 0x20, 0x01, 0xc2,
0x02, 0x03, 0x20, 0x40, 0xe6, 0x06, 0x0d, 0x01, 0xa2, 0xa2, 0x10, 0xe3, 0x05, 0xd8, 0x00, 0x67,
0xd8, 0x5d, 0xc4, 0x01, 0x6e, 0x80, 0x00, 0x68, 0x03, 0x0c, 0x00, 0x00, 0x00, 0x30, 0x00, 0x0b,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8c
};


void modifyEdid(vector<BYTE>& edid) {
	if (edid.size() < 12) {
		return;
	}

	edid[8] = 0x36;
	edid[9] = 0x94;
	edid[10] = 0x37;
	edid[11] = 0x13;
}



BYTE calculateChecksum(const std::vector<BYTE>& edid) {
	int sum = 0;
	for (int i = 0; i < 127; ++i) {
		sum += edid[i];
	}
	sum %= 256;
	if (sum != 0) {
		sum = 256 - sum;
	}
	return static_cast<BYTE>(sum);
	// check sum calculations. We dont need to include old checksum in calculation, so we only read up to the byte before.
	// Anything after the checksum bytes arent part of the checksum - a flaw with edid managment, not with us
}

void updateCeaExtensionCount(vector<BYTE>& edid, int count) {
	edid[126] = static_cast<BYTE>(count);
}

vector<BYTE> loadEdid(const string& filePath) {
	if (g_settings.edid.custom_edid) {
		g_log.Message(Refactoring::LogType::Info, "Attempting to use user Edid");
	}
	else {
		g_log.Message(Refactoring::LogType::Info, "Using hardcoded edid");
		return hardcodedEdid;
	}

	ifstream file(filePath, ios::binary | ios::ate);
	if (!file) {
		g_log.Message(Refactoring::LogType::Info, "No custom edid found, using hardcoded edid");
		return hardcodedEdid;
	}

	streamsize size = file.tellg();
	file.seekg(0, ios::beg);

	vector<BYTE> buffer(size);
	if (file.read((char*)buffer.data(), size)) {
		//calculate checksum and compare it to 127 byte, if false then return hardcoded if true then return buffer to prevent loading borked edid.
		BYTE calculatedChecksum = calculateChecksum(buffer);
		if (calculatedChecksum != buffer[127]) {
			g_log.Message(Refactoring::LogType::Error, "Custom edid failed due to invalid checksum");
			g_log.Message(Refactoring::LogType::Info, "Using hardcoded edid");
			return hardcodedEdid;
		}

		if (g_settings.edid.edid_cea_override) {
			if (buffer.size() == 256) {
				for (int i = 128; i < 256; ++i) {
					buffer[i] = hardcodedEdid[i];
				}
				updateCeaExtensionCount(buffer, 1);
			}
			else if (buffer.size() == 128) {
				buffer.insert(buffer.end(), hardcodedEdid.begin() + 128, hardcodedEdid.end());
				updateCeaExtensionCount(buffer, 1);
			}
		}

		g_log.Message(Refactoring::LogType::Info, "Using custom edid");
		return buffer;
	}
	else {
		g_log.Message(Refactoring::LogType::Info, "Using hardcoded edid");
		return hardcodedEdid;
	}
}

int maincalc() {
	vector<BYTE> edid = loadEdid(Refactoring::WStringToString(confpath) + "\\user_edid.bin");

	if (!g_settings.edid.prevent_manufacturer_spoof) modifyEdid(edid);
	BYTE checksum = calculateChecksum(edid);
	edid[127] = checksum;
	// Setting this variable is depricated, hardcoded edid is either returned or custom in loading edid function
	IndirectDeviceContext::s_KnownMonitorEdid = edid;
	return 0;
}

std::shared_ptr<Direct3DDevice> IndirectDeviceContext::GetOrCreateDevice(LUID RenderAdapter)
{
	std::shared_ptr<Direct3DDevice> Device;

	{
		std::lock_guard<std::mutex> lock(s_DeviceCacheMutex);
		auto it = s_DeviceCache.find(RenderAdapter);
		if (it != s_DeviceCache.end()) {
			Device = it->second;
			if (Device) {
				g_log.Message(Refactoring::LogType::Debug, std::format("Reusing cached Direct3DDevice for LUID {} - {}", RenderAdapter.HighPart, RenderAdapter.LowPart).c_str());
				return Device;
			} else {
				g_log.Message(Refactoring::LogType::Debug, std::format("Cached Direct3DDevice is null for LUID {} - {}, removing from cache", RenderAdapter.HighPart, RenderAdapter.LowPart)
								.c_str());
				s_DeviceCache.erase(it);
			}
		}
	}
	
	Device = make_shared<Direct3DDevice>(RenderAdapter);
	if (FAILED(Device->Init())) {
		g_log.Message(Refactoring::LogType::Error, "Failed to initialize new Direct3DDevice");
		return nullptr;
	}

	{
		std::lock_guard<std::mutex> lock(s_DeviceCacheMutex);
		s_DeviceCache[RenderAdapter] = Device;
		auto msg = std::format("Created and cached new Direct3DDevice for LUID {} - {} (cache size now: {})", RenderAdapter.HighPart,
							   RenderAdapter.LowPart, s_DeviceCache.size());
		g_log.Message(Refactoring::LogType::Debug, msg.c_str());
	}

	return Device;
}

void IndirectDeviceContext::CleanupExpiredDevices()
{
	std::lock_guard<std::mutex> lock(s_DeviceCacheMutex);
	
	int removed = 0;
	for (auto it = s_DeviceCache.begin(); it != s_DeviceCache.end();) {
		// With shared_ptr cache, we only remove null devices (shouldn't happen)
		if (!it->second) {
			it = s_DeviceCache.erase(it);
			removed++;
		} else {
			++it;
		}
	}
	
	if (removed > 0) {
		g_log.Message(Refactoring::LogType::Debug, std::format("Cleaned up {} null Direct3DDevice references from cache", removed).c_str());
	}
}

IndirectDeviceContext::IndirectDeviceContext(_In_ WDFDEVICE WdfDevice) :
	m_WdfDevice(WdfDevice),
	m_Adapter(nullptr),
	m_Monitor(nullptr),
	m_Monitor2(nullptr)
{}

IndirectDeviceContext::~IndirectDeviceContext()
{
	std::map<IDDCX_MONITOR, std::unique_ptr<SwapChainProcessor>> processingThreads;

	g_log.Message(Refactoring::LogType::Debug, "Destroying IndirectDeviceContext. Releasing per-monitor processing threads.");

	{
		std::lock_guard<std::mutex> lock(m_ProcessingThreadsMutex);
		processingThreads.swap(m_ProcessingThreads);
	}

	g_log.Message(Refactoring::LogType::Debug, std::format("Released {} monitor processing thread(s).", processingThreads.size()).c_str());
}

#define NUM_VIRTUAL_DISPLAYS 1   //What is this even used for ?? Its never referenced

void IndirectDeviceContext::InitAdapter()
{
	maincalc();
	stringstream logStream;

	// ==============================
	// TODO: Update the below diagnostic information in accordance with the target hardware. The strings and version
	// numbers are used for telemetry and may be displayed to the user in some situations.
	//
	// This is also where static per-adapter capabilities are determined.
	// ==============================

	g_log.Message(Refactoring::LogType::Debug, "Initializing adapter...");

	IDDCX_ADAPTER_CAPS AdapterCaps = {};
	AdapterCaps.Size = sizeof(AdapterCaps);

	if (IDD_IS_FUNCTION_AVAILABLE(IddCxSwapChainReleaseAndAcquireBuffer2)) {
		AdapterCaps.Flags = IDDCX_ADAPTER_FLAGS_CAN_PROCESS_FP16;
		g_log.Message(Refactoring::LogType::Debug, "FP16 processing capability detected.");
	}

	// Declare basic feature support for the adapter (required)
	AdapterCaps.MaxMonitorsSupported = numVirtualDisplays;
	AdapterCaps.EndPointDiagnostics.Size = sizeof(AdapterCaps.EndPointDiagnostics);
	AdapterCaps.EndPointDiagnostics.GammaSupport = IDDCX_FEATURE_IMPLEMENTATION_NONE;
	AdapterCaps.EndPointDiagnostics.TransmissionType = IDDCX_TRANSMISSION_TYPE_WIRED_OTHER;

	// Declare your device strings for telemetry (required)
	AdapterCaps.EndPointDiagnostics.pEndPointFriendlyName = L"VirtualDisplayDriver Device";
	AdapterCaps.EndPointDiagnostics.pEndPointManufacturerName = L"MikeTheTech";
	AdapterCaps.EndPointDiagnostics.pEndPointModelName = L"VirtualDisplayDriver Model";

	// Declare your hardware and firmware versions (required)
	IDDCX_ENDPOINT_VERSION Version = {};
	Version.Size = sizeof(Version);
	Version.MajorVer = 1;
	AdapterCaps.EndPointDiagnostics.pFirmwareVersion = &Version;
	AdapterCaps.EndPointDiagnostics.pHardwareVersion = &Version;

	auto msg =
		std::format("Adapter Caps Initialized:\n  Max Monitors Supported: {}\n  Gamma Support: {}\n  Transmission Type: {}\n  Friendly Name: {}\n  "
					"Manufacturer Name: {}\n  Model Name: {}\n  Firmware Version: {}\n  Hardware Version: {}",
					AdapterCaps.MaxMonitorsSupported, static_cast<int>(AdapterCaps.EndPointDiagnostics.GammaSupport), static_cast<int>(AdapterCaps.EndPointDiagnostics.TransmissionType),
					Refactoring::WStringToString(AdapterCaps.EndPointDiagnostics.pEndPointFriendlyName), Refactoring::WStringToString(AdapterCaps.EndPointDiagnostics.pEndPointManufacturerName),
					Refactoring::WStringToString(AdapterCaps.EndPointDiagnostics.pEndPointModelName), Version.MajorVer, Version.MajorVer);

	g_log.Message(Refactoring::LogType::Debug, msg.c_str());

	// Initialize a WDF context that can store a pointer to the device context object
	WDF_OBJECT_ATTRIBUTES Attr;
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attr, IndirectDeviceContextWrapper);

	IDARG_IN_ADAPTER_INIT AdapterInit = {};
	AdapterInit.WdfDevice = m_WdfDevice;
	AdapterInit.pCaps = &AdapterCaps;
	AdapterInit.ObjectAttributes = &Attr;

	// Start the initialization of the adapter, which will trigger the AdapterFinishInit callback later
	IDARG_OUT_ADAPTER_INIT AdapterInitOut;
	NTSTATUS Status = IddCxAdapterInitAsync(&AdapterInit, &AdapterInitOut);

	g_log.Message(Refactoring::LogType::Debug, std::format("Adapter Initialization Status: {}", Status).c_str());
	logStream.str("");

	if (NT_SUCCESS(Status))
	{
		// Store a reference to the WDF adapter handle
		m_Adapter = AdapterInitOut.AdapterObject;
		g_log.Message(Refactoring::LogType::Debug, "Adapter handle stored successfully.");

		// Store the device context object into the WDF object context
		auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(AdapterInitOut.AdapterObject);
		pContext->pContext = this;
	}
	else {
		g_log.Message(Refactoring::LogType::Error, std::format("Failed to initialize adapter. Status: {}", Status).c_str());
	}
}

void IndirectDeviceContext::FinishInit()
{
	Options.Adapter.apply(m_Adapter);
	g_log.Message(Refactoring::LogType::Info, "Applied Adapter configs.");
	for (unsigned int i = 0; i < numVirtualDisplays; i++) {
		CreateMonitor(i);
	}
}

void IndirectDeviceContext::CreateMonitor(unsigned int index) {
	wstring logMessage = L"Creating Monitor: " + to_wstring(index + 1);
	string narrowLogMessage = Refactoring::WStringToString(logMessage);
	g_log.Message(Refactoring::LogType::Info, narrowLogMessage.c_str());

	// ==============================
	// TODO: In a real driver, the EDID should be retrieved dynamically from a connected physical monitor. The EDID
	// provided here is purely for demonstration, as it describes only 640x480 @ 60 Hz and 800x600 @ 60 Hz. Monitor
	// manufacturers are required to correctly fill in physical monitor attributes in order to allow the OS to optimize
	// settings like viewing distance and scale factor. Manufacturers should also use a unique serial number every
	// single device to ensure the OS can tell the monitors apart.
	// ==============================

	WDF_OBJECT_ATTRIBUTES Attr;
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attr, IndirectDeviceContextWrapper);

	IDDCX_MONITOR_INFO MonitorInfo = {};
	MonitorInfo.Size = sizeof(MonitorInfo);
	MonitorInfo.MonitorType = DISPLAYCONFIG_OUTPUT_TECHNOLOGY_HDMI;
	MonitorInfo.ConnectorIndex = index;
	MonitorInfo.MonitorDescription.Size = sizeof(MonitorInfo.MonitorDescription);
	MonitorInfo.MonitorDescription.Type = IDDCX_MONITOR_DESCRIPTION_TYPE_EDID;
	//MonitorInfo.MonitorDescription.DataSize = sizeof(s_KnownMonitorEdid);        can no longer use size of as converted to vector
	if (s_KnownMonitorEdid.size() > UINT_MAX)
	{
		g_log.Message(Refactoring::LogType::Error, "Edid size passes UINT_Max, escape to prevent loading borked display");
	}
	else
	{
		MonitorInfo.MonitorDescription.DataSize = static_cast<UINT>(s_KnownMonitorEdid.size());
	}
	//MonitorInfo.MonitorDescription.pData = const_cast<BYTE*>(s_KnownMonitorEdid);
	// Changed from using const_cast to data() to safely access the EDID data.
	// This improves type safety and code readability, as it eliminates the need for casting 
	// and ensures we are directly working with the underlying container of known monitor EDID data.
	MonitorInfo.MonitorDescription.pData = IndirectDeviceContext::s_KnownMonitorEdid.data();


	// ==============================
	// TODO: The monitor's container ID should be distinct from "this" device's container ID if the monitor is not
	// permanently attached to the display adapter device object. The container ID is typically made unique for each
	// monitor and can be used to associate the monitor with other devices, like audio or input devices. In this
	// sample we generate a random container ID GUID, but it's best practice to choose a stable container ID for a
	// unique monitor or to use "this" device's container ID for a permanent/integrated monitor.
	// ==============================

	// Create a container ID
	CoCreateGuid(&MonitorInfo.MonitorContainerId);
	g_log.Message(Refactoring::LogType::Debug, "Created container ID");

	IDARG_IN_MONITORCREATE MonitorCreate = {};
	MonitorCreate.ObjectAttributes = &Attr;
	MonitorCreate.pMonitorInfo = &MonitorInfo;

	// Create a monitor object with the specified monitor descriptor
	IDARG_OUT_MONITORCREATE MonitorCreateOut;
	NTSTATUS Status = IddCxMonitorCreate(m_Adapter, &MonitorCreate, &MonitorCreateOut);
	if (NT_SUCCESS(Status))
	{
		g_log.Message(Refactoring::LogType::Debug, "Monitor created successfully.");
		m_Monitor = MonitorCreateOut.MonitorObject;

		// Associate the monitor with this device context
		auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(MonitorCreateOut.MonitorObject);
		pContext->pContext = this;

		// Tell the OS that the monitor has been plugged in
		IDARG_OUT_MONITORARRIVAL ArrivalOut;
		Status = IddCxMonitorArrival(m_Monitor, &ArrivalOut);
		if (NT_SUCCESS(Status))
		{
			g_log.Message(Refactoring::LogType::Debug, "Monitor arrival successfully reported.");
		}
		else
		{
			g_log.Message(Refactoring::LogType::Error, std::format("Failed to report monitor arrival. Status: {}", Status).c_str());
		}
	}
	else
	{
		g_log.Message(Refactoring::LogType::Error, std::format("Failed to create monitor. Status: {}", Status).c_str());
	}
}

void IndirectDeviceContext::AssignSwapChain(IDDCX_MONITOR Monitor, IDDCX_SWAPCHAIN SwapChain, LUID RenderAdapter, HANDLE NewFrameEvent)
{
	// Only cleanup expired devices periodically, not on every assignment
	static int assignmentCount = 0;
	if (++assignmentCount % 10 == 0) {
		CleanupExpiredDevices();
	}

	auto Device = GetOrCreateDevice(RenderAdapter);
	if (!Device)
	{
		g_log.Message(Refactoring::LogType::Error, "Failed to get or create Direct3DDevice, deleting existing swap-chain.");
		WdfObjectDelete(SwapChain);
		return;
	}
	else
	{
		std::unique_ptr<SwapChainProcessor> previousProcessor;
		auto newProcessor = std::make_unique<SwapChainProcessor>(SwapChain, Device, NewFrameEvent);

		{
			std::lock_guard<std::mutex> lock(m_ProcessingThreadsMutex);
			auto& processorSlot = m_ProcessingThreads[Monitor];
			previousProcessor = std::move(processorSlot);
			processorSlot = std::move(newProcessor);
		}

		if (previousProcessor) {
			g_log.Message(Refactoring::LogType::Debug, "Replaced existing processing thread for this monitor only.");
		}
		else {
			g_log.Message(Refactoring::LogType::Debug, "Created a new processing thread for this monitor.");
		}

		if (g_settings.cursor.hardware_cursor){
			HANDLE mouseEvent = CreateEventA(
				nullptr, 
				false,   
				false,   
				"VirtualDisplayDriverMouse"
			);

			if (!mouseEvent)
			{
				g_log.Message(Refactoring::LogType::Error, "Failed to create mouse event. No hardware cursor supported!");
				return;
			}

			IDDCX_CURSOR_CAPS cursorInfo = {};
			cursorInfo.Size = sizeof(cursorInfo);
			cursorInfo.ColorXorCursorSupport = IDDCX_XOR_CURSOR_SUPPORT_FULL; 
			cursorInfo.AlphaCursorSupport = g_settings.cursor.alpha_cursor_support;

			cursorInfo.MaxX = g_settings.cursor.max_x;       //Apparently in most cases 128 is fine but for safe guarding we will go 512, older intel cpus may be limited to 64x64
			cursorInfo.MaxY = g_settings.cursor.max_y;

			//DirectXDevice->QueryMaxCursorSize(&cursorInfo.MaxX, &cursorInfo.MaxY);                 Experimental to get max cursor size - THIS IS NTO WORKING CODE


			IDARG_IN_SETUP_HWCURSOR hwCursor = {};
			hwCursor.CursorInfo = cursorInfo;
			hwCursor.hNewCursorDataAvailable = mouseEvent;

			NTSTATUS Status = IddCxMonitorSetupHardwareCursor(
				Monitor,
				&hwCursor
			);

			if (FAILED(Status))
			{
				CloseHandle(mouseEvent); 
				return;
			}

			g_log.Message(Refactoring::LogType::Debug, "Hardware cursor setup completed successfully.");
		}
		else {
			g_log.Message(Refactoring::LogType::Debug, "Hardware cursor is disabled, Skipped creation.");
		}
		// At this point, the swap-chain is set up and the hardware cursor is enabled
		// Further swap-chain and cursor processing will occur in the new processing thread.
	}
}


void IndirectDeviceContext::UnassignSwapChain(IDDCX_MONITOR Monitor)
{
	std::unique_ptr<SwapChainProcessor> processorToStop;

	{
		std::lock_guard<std::mutex> lock(m_ProcessingThreadsMutex);
		auto it = m_ProcessingThreads.find(Monitor);
		if (it != m_ProcessingThreads.end())
		{
			processorToStop = std::move(it->second);
			m_ProcessingThreads.erase(it);
		}
	}

	if (processorToStop)
	{
		g_log.Message(Refactoring::LogType::Info, "Unassigning swapchain for one monitor. Its processing thread will be stopped.");
	}
	else
	{
		g_log.Message(Refactoring::LogType::Warning, "UnassignSwapChain called for a monitor without an active processing thread.");
	}
}

#pragma endregion

#pragma region DDI Callbacks

_Use_decl_annotations_
NTSTATUS VirtualDisplayDriverAdapterInitFinished(IDDCX_ADAPTER AdapterObject, const IDARG_IN_ADAPTER_INIT_FINISHED* pInArgs)
{
	// This is called when the OS has finished setting up the adapter for use by the IddCx driver. It's now possible
	// to report attached monitors.

	auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(AdapterObject);
	if (NT_SUCCESS(pInArgs->AdapterInitStatus))
	{
		pContext->pContext->FinishInit();
		g_log.Message(Refactoring::LogType::Debug, "Adapter initialization finished successfully.");
	}
	else
	{
		g_log.Message(Refactoring::LogType::Error, std::format("Adapter initialization failed. Status: {}", pInArgs->AdapterInitStatus).c_str());
	}
	g_log.Message(Refactoring::LogType::Info, "Finished Setting up adapter.");
	

	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS VirtualDisplayDriverAdapterCommitModes(IDDCX_ADAPTER AdapterObject, const IDARG_IN_COMMITMODES* pInArgs)
{
	UNREFERENCED_PARAMETER(AdapterObject);
	UNREFERENCED_PARAMETER(pInArgs);

	// For the sample, do nothing when modes are picked - the swap-chain is taken care of by IddCx

	// ==============================
	// TODO: In a real driver, this function would be used to reconfigure the device to commit the new modes. Loop
	// through pInArgs->pPaths and look for IDDCX_PATH_FLAGS_ACTIVE. Any path not active is inactive (e.g. the monitor
	// should be turned off).
	// ==============================

	return STATUS_SUCCESS;
}
_Use_decl_annotations_
NTSTATUS VirtualDisplayDriverParseMonitorDescription(const IDARG_IN_PARSEMONITORDESCRIPTION* pInArgs, IDARG_OUT_PARSEMONITORDESCRIPTION* pOutArgs)
{
	// ==============================
	// TODO: In a real driver, this function would be called to generate monitor modes for an EDID by parsing it. In
	// this sample driver, we hard-code the EDID, so this function can generate known modes.
	// ==============================

	stringstream logStream;
	g_log.Message(Refactoring::LogType::Debug, std::format("Parsing monitor description. Input buffer count: {}", pInArgs->MonitorModeBufferInputCount).c_str());

	RebuildKnownMonitorModesCache();
	pOutArgs->MonitorModeBufferOutputCount = (UINT)monitorModes.size();

	g_log.Message(Refactoring::LogType::Debug, std::format("Number of monitor modes generated: {}", monitorModes.size()).c_str());

	if (pInArgs->MonitorModeBufferInputCount < monitorModes.size())
	{
		g_log.Message(Refactoring::LogType::Warning,
			   std::format("Buffer too small. Input count: {}, Required: {}", pInArgs->MonitorModeBufferInputCount, monitorModes.size()).c_str());
		// Return success if there was no buffer, since the caller was only asking for a count of modes
		return (pInArgs->MonitorModeBufferInputCount > 0) ? STATUS_BUFFER_TOO_SMALL : STATUS_SUCCESS;
	}
	else
	{
		// Copy the known modes to the output buffer
		for (DWORD ModeIndex = 0; ModeIndex < monitorModes.size(); ModeIndex++)
		{
			pInArgs->pMonitorModes[ModeIndex].Size = sizeof(IDDCX_MONITOR_MODE);
			pInArgs->pMonitorModes[ModeIndex].Origin = IDDCX_MONITOR_MODE_ORIGIN_MONITORDESCRIPTOR;
			pInArgs->pMonitorModes[ModeIndex].MonitorVideoSignalInfo = s_KnownMonitorModes2[ModeIndex];
		}

		// Set the preferred mode as represented in the EDID
		pOutArgs->PreferredMonitorModeIdx = 0;
		g_log.Message(Refactoring::LogType::Debug, "Monitor description parsed successfully.");
		return STATUS_SUCCESS;
	}
}

_Use_decl_annotations_
NTSTATUS VirtualDisplayDriverMonitorGetDefaultModes(IDDCX_MONITOR MonitorObject, const IDARG_IN_GETDEFAULTDESCRIPTIONMODES* pInArgs, IDARG_OUT_GETDEFAULTDESCRIPTIONMODES* pOutArgs)
{
	UNREFERENCED_PARAMETER(MonitorObject);
	UNREFERENCED_PARAMETER(pInArgs);
	UNREFERENCED_PARAMETER(pOutArgs);

	// Should never be called since we create a single monitor with a known EDID in this sample driver.

	// ==============================
	// TODO: In a real driver, this function would be called to generate monitor modes for a monitor with no EDID.
	// Drivers should report modes that are guaranteed to be supported by the transport protocol and by nearly all
	// monitors (such 640x480, 800x600, or 1024x768). If the driver has access to monitor modes from a descriptor other
	// than an EDID, those modes would also be reported here.
	// ==============================

	return STATUS_NOT_IMPLEMENTED;
}

/// <summary>
/// Creates a target mode from the fundamental mode attributes.
/// </summary>
void CreateTargetMode(DISPLAYCONFIG_VIDEO_SIGNAL_INFO& Mode, UINT Width, UINT Height, UINT VSyncNum, UINT VSyncDen)
{
	stringstream logStream;

	Mode.totalSize.cx = Mode.activeSize.cx = Width;
	Mode.totalSize.cy = Mode.activeSize.cy = Height;
	Mode.AdditionalSignalInfo.vSyncFreqDivider = 1;
	Mode.AdditionalSignalInfo.videoStandard = 255;
	Mode.vSyncFreq.Numerator = VSyncNum;
	Mode.vSyncFreq.Denominator = VSyncDen;
	Mode.hSyncFreq.Numerator = VSyncNum * Height;
	Mode.hSyncFreq.Denominator = VSyncDen;
	Mode.scanLineOrdering = DISPLAYCONFIG_SCANLINE_ORDERING_PROGRESSIVE;
	Mode.pixelRate = VSyncNum * Width * Height / VSyncDen;

	logStream << "[CreateTargetMode] Target mode configured with:"
		<< "\n  Total Size: (" << Mode.totalSize.cx << ", " << Mode.totalSize.cy << ")"
		<< "\n  Active Size: (" << Mode.activeSize.cx << ", " << Mode.activeSize.cy << ")"
		<< "\n  vSync Frequency: " << Mode.vSyncFreq.Numerator << "/" << Mode.vSyncFreq.Denominator
		<< "\n  hSync Frequency: " << Mode.hSyncFreq.Numerator << "/" << Mode.hSyncFreq.Denominator
		<< "\n  Pixel Rate: " << Mode.pixelRate
		<< "\n  Scan Line Ordering: " << Mode.scanLineOrdering;
	g_log.Message(Refactoring::LogType::Debug, logStream.str().c_str());
}

void CreateTargetMode(IDDCX_TARGET_MODE& Mode, UINT Width, UINT Height, UINT VSyncNum, UINT VSyncDen)
{
	Mode.Size = sizeof(Mode);
	CreateTargetMode(Mode.TargetVideoSignalInfo.targetVideoSignalInfo, Width, Height, VSyncNum, VSyncDen);
}

void CreateTargetMode2(IDDCX_TARGET_MODE2& Mode, UINT Width, UINT Height, UINT VSyncNum, UINT VSyncDen)
{
	auto msg = std::format("[CreateTargetMode2] Creating IDDCX_TARGET_MODE2 with Width: {}, Height: {}, VSyncNum: {}, VSyncDen {}", Width, Height, VSyncNum, VSyncDen);
	g_log.Message(Refactoring::LogType::Debug, msg.c_str());

	Mode.Size = sizeof(Mode);

	if (g_settings.colours.color_format == "RGB")
	{
		Mode.BitsPerComponent.Rgb = g_colours_iddcx.SDR_COLOR | g_colours_iddcx.HDR_COLOR;
	}
	else if (g_settings.colours.color_format == "YCbCr444") {
		Mode.BitsPerComponent.YCbCr444 = g_colours_iddcx.SDR_COLOR | g_colours_iddcx.HDR_COLOR;
	}
	else if (g_settings.colours.color_format == "YCbCr422") {
		Mode.BitsPerComponent.YCbCr422 = g_colours_iddcx.SDR_COLOR | g_colours_iddcx.HDR_COLOR; 
	}
	else if (g_settings.colours.color_format == "YCbCr420") {
		Mode.BitsPerComponent.YCbCr420 = g_colours_iddcx.SDR_COLOR | g_colours_iddcx.HDR_COLOR; 
	}
	else {
		Mode.BitsPerComponent.Rgb = g_colours_iddcx.SDR_COLOR | g_colours_iddcx.HDR_COLOR; // Default to RGB
	}

	g_log.Message(Refactoring::LogType::Debug, std::format("IDDCX_TARGET_MODE2 configured with Size: {} and colour format {}", Mode.Size, g_settings.colours.color_format).c_str());

	CreateTargetMode(Mode.TargetVideoSignalInfo.targetVideoSignalInfo, Width, Height, VSyncNum, VSyncDen);
}

_Use_decl_annotations_
NTSTATUS VirtualDisplayDriverMonitorQueryModes(IDDCX_MONITOR MonitorObject, const IDARG_IN_QUERYTARGETMODES* pInArgs, IDARG_OUT_QUERYTARGETMODES* pOutArgs)////////////////////////////////////////////////////////////////////////////////
{
	UNREFERENCED_PARAMETER(MonitorObject);

	vector<IDDCX_TARGET_MODE> TargetModes(monitorModes.size());

	stringstream logStream;
	logStream << "Creating target modes. Number of monitor modes: " << monitorModes.size();
	g_log.Message(Refactoring::LogType::Debug, logStream.str().c_str());

	// Create a set of modes supported for frame processing and scan-out. These are typically not based on the
	// monitor's descriptor and instead are based on the static processing capability of the device. The OS will
	// report the available set of modes for a given output as the intersection of monitor modes with target modes.

	for (int i = 0; i < monitorModes.size(); i++) {
		CreateTargetMode(TargetModes[i], monitorModes[i].width, monitorModes[i].height, monitorModes[i].refresh_num, monitorModes[i].refresh_den);

		logStream.str("");
		logStream << "Created target mode " << i << ": Width = " << monitorModes[i].width
			<< ", Height = " << monitorModes[i].height
			<< ", VSync = " << monitorModes[i].refresh_num; //qui è sbagliato
		g_log.Message(Refactoring::LogType::Debug, logStream.str().c_str());
	}

	pOutArgs->TargetModeBufferOutputCount = (UINT)TargetModes.size();

	logStream.str("");
	logStream << "Number of target modes to output: " << pOutArgs->TargetModeBufferOutputCount;
	g_log.Message(Refactoring::LogType::Debug, logStream.str().c_str());

	if (pInArgs->TargetModeBufferInputCount >= TargetModes.size())
	{
		logStream.str("");
		logStream << "Copying target modes to output buffer.";
		g_log.Message(Refactoring::LogType::Debug, logStream.str().c_str());
		copy(TargetModes.begin(), TargetModes.end(), pInArgs->pTargetModes);
	}
	else {
		logStream.str("");
		logStream << "Input buffer too small. Required: " << TargetModes.size()
			<< ", Provided: " << pInArgs->TargetModeBufferInputCount;
		g_log.Message(Refactoring::LogType::Warning, logStream.str().c_str());
	}

	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS VirtualDisplayDriverMonitorAssignSwapChain(IDDCX_MONITOR MonitorObject, const IDARG_IN_SETSWAPCHAIN* pInArgs)
{
	stringstream logStream;
	logStream << "Assigning swap chain:"
		<< "\n  hSwapChain: " << pInArgs->hSwapChain
		<< "\n  RenderAdapterLuid: " << pInArgs->RenderAdapterLuid.LowPart << "-" << pInArgs->RenderAdapterLuid.HighPart
		<< "\n  hNextSurfaceAvailable: " << pInArgs->hNextSurfaceAvailable;
	g_log.Message(Refactoring::LogType::Debug, logStream.str().c_str());
	auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(MonitorObject);
	pContext->pContext->AssignSwapChain(MonitorObject, pInArgs->hSwapChain, pInArgs->RenderAdapterLuid, pInArgs->hNextSurfaceAvailable);
	g_log.Message(Refactoring::LogType::Debug, "Swap chain assigned successfully.");
	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS VirtualDisplayDriverMonitorUnassignSwapChain(IDDCX_MONITOR MonitorObject)
{
	g_log.Message(Refactoring::LogType::Debug, std::format("Unassigning swap chain for monitor object: {:p}", static_cast<void *>(MonitorObject)).c_str());
	auto* pContext = WdfObjectGet_IndirectDeviceContextWrapper(MonitorObject);
	pContext->pContext->UnassignSwapChain(MonitorObject);
	g_log.Message(Refactoring::LogType::Debug, "Swap chain unassigned successfully.");
	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS VirtualDisplayDriverEvtIddCxAdapterQueryTargetInfo(
	IDDCX_ADAPTER AdapterObject,
	IDARG_IN_QUERYTARGET_INFO* pInArgs,
	IDARG_OUT_QUERYTARGET_INFO* pOutArgs
)
{
	g_log.Message(Refactoring::LogType::Debug, std::format("Querying target info for adapter object: {:p}", static_cast<void *>(AdapterObject)).c_str());

	UNREFERENCED_PARAMETER(pInArgs);

	pOutArgs->TargetCaps = IDDCX_TARGET_CAPS_HIGH_COLOR_SPACE | IDDCX_TARGET_CAPS_WIDE_COLOR_SPACE;

	if (g_settings.colours.color_format == "RGB")
	{
		pOutArgs->DitheringSupport.Rgb = g_colours_iddcx.SDR_COLOR | g_colours_iddcx.HDR_COLOR;
	}
	else if (g_settings.colours.color_format == "YCbCr444")
	{
		pOutArgs->DitheringSupport.YCbCr444 = g_colours_iddcx.SDR_COLOR | g_colours_iddcx.HDR_COLOR;
	}
	else if (g_settings.colours.color_format == "YCbCr422")
	{
		pOutArgs->DitheringSupport.YCbCr422 = g_colours_iddcx.SDR_COLOR | g_colours_iddcx.HDR_COLOR; 
	}
	else if (g_settings.colours.color_format == "YCbCr420")
	{
		pOutArgs->DitheringSupport.YCbCr420 = g_colours_iddcx.SDR_COLOR | g_colours_iddcx.HDR_COLOR; 
	}
	else {
		pOutArgs->DitheringSupport.Rgb = g_colours_iddcx.SDR_COLOR | g_colours_iddcx.HDR_COLOR; // Default to RGB
	}

	g_log.Message(Refactoring::LogType::Debug, std::format("Target capabilities set to: {}\nDithering support colour format set to: {}", 
		static_cast<int>(pOutArgs->TargetCaps), g_settings.colours.color_format).c_str());

	return STATUS_SUCCESS;
}

_Use_decl_annotations_ 
NTSTATUS VirtualDisplayDriverEvtIddCxMonitorSetDefaultHdrMetadata(IDDCX_MONITOR MonitorObject,
																						 const IDARG_IN_MONITOR_SET_DEFAULT_HDR_METADATA *pInArgs)
{
	UNREFERENCED_PARAMETER(MonitorObject);
	UNREFERENCED_PARAMETER(pInArgs);

	// Check if HDR metadata processing is enabled
	if (!g_settings.hdr_advanced.static_metadata_enabled) {
		return STATUS_SUCCESS;
	}

	// Virtual display: nessun hardware su cui applicare la hdr metadata.
	// Le caratteristiche hdr del monitor sono comunicate via EDID/monitor description.
	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS VirtualDisplayDriverEvtIddCxParseMonitorDescription2(
	const IDARG_IN_PARSEMONITORDESCRIPTION2* pInArgs,
	IDARG_OUT_PARSEMONITORDESCRIPTION* pOutArgs
)
{
	// ==============================
	// TODO: In a real driver, this function would be called to generate monitor modes for an EDID by parsing it. In
	// this sample driver, we hard-code the EDID, so this function can generate known modes.
	// ==============================

	stringstream logStream;
	auto msg1 = std::format("Parsing monitor description:\n  MonitorModeBufferInputCount: {}\n  pMonitorModes: {}",
						   pInArgs->MonitorModeBufferInputCount, pInArgs->pMonitorModes ? "Valid" : "Null");
	g_log.Message(Refactoring::LogType::Debug, msg1.c_str());
	g_log.Message(Refactoring::LogType::Info, "Monitor Modes:");
	for (const auto& mode : monitorModes)
	{
		g_log.Message(Refactoring::LogType::Debug,
			   std::format("\n Mode - Width : {}, Height: {}, RefreshRate: {}", mode.width, mode.height, mode.refresh_num).c_str());
	}

	RebuildKnownMonitorModesCache();
	pOutArgs->MonitorModeBufferOutputCount = (UINT)monitorModes.size();

	if (pInArgs->MonitorModeBufferInputCount < monitorModes.size())
	{
		// Return success if there was no buffer, since the caller was only asking for a count of modes
		return (pInArgs->MonitorModeBufferInputCount > 0) ? STATUS_BUFFER_TOO_SMALL : STATUS_SUCCESS;
	}
	else
	{
		// Copy the known modes to the output buffer
		if (pInArgs->pMonitorModes == nullptr) {
			g_log.Message(Refactoring::LogType::Error, "pMonitorModes is null but buffer size is sufficient");
			return STATUS_INVALID_PARAMETER;
		}
		

		g_log.Message(Refactoring::LogType::Info, "Writing monitor modes to output buffer:");
		for (DWORD ModeIndex = 0; ModeIndex < monitorModes.size(); ModeIndex++)
		{
			pInArgs->pMonitorModes[ModeIndex].Size = sizeof(IDDCX_MONITOR_MODE2);
			pInArgs->pMonitorModes[ModeIndex].Origin = IDDCX_MONITOR_MODE_ORIGIN_MONITORDESCRIPTOR;
			pInArgs->pMonitorModes[ModeIndex].MonitorVideoSignalInfo = s_KnownMonitorModes2[ModeIndex];


			if (g_settings.colours.color_format == "RGB")
			{
				pInArgs->pMonitorModes[ModeIndex].BitsPerComponent.Rgb =
					g_colours_iddcx.SDR_COLOR | g_colours_iddcx.HDR_COLOR;
				
			}
			else if (g_settings.colours.color_format == "YCbCr444")
			{
				pInArgs->pMonitorModes[ModeIndex].BitsPerComponent.YCbCr444 =
					g_colours_iddcx.SDR_COLOR | g_colours_iddcx.HDR_COLOR;
			}
			else if (g_settings.colours.color_format == "YCbCr422")
			{
				pInArgs->pMonitorModes[ModeIndex].BitsPerComponent.YCbCr422 =
					g_colours_iddcx.SDR_COLOR | g_colours_iddcx.HDR_COLOR;
			}
			else if (g_settings.colours.color_format == "YCbCr420")
			{
				pInArgs->pMonitorModes[ModeIndex].BitsPerComponent.YCbCr420 =
					g_colours_iddcx.SDR_COLOR | g_colours_iddcx.HDR_COLOR;
			}
			else {
				pInArgs->pMonitorModes[ModeIndex].BitsPerComponent.Rgb =
					g_colours_iddcx.SDR_COLOR | g_colours_iddcx.HDR_COLOR; // Default to RGB
			}
			auto msg2 = std::format("\n  ModeIndex: {}\n  Size: {}\n  Origin: {}\n  Colour Format: {}", ModeIndex,
								   pInArgs->pMonitorModes[ModeIndex].Size, static_cast<int>(pInArgs->pMonitorModes[ModeIndex].Origin), g_settings.colours.color_format);
			g_log.Message(Refactoring::LogType::Debug, msg2.c_str());
		}

		// Set the preferred mode as represented in the EDID
		pOutArgs->PreferredMonitorModeIdx = 0;

		return STATUS_SUCCESS;
	}
}

_Use_decl_annotations_
NTSTATUS VirtualDisplayDriverEvtIddCxMonitorQueryTargetModes2(
	IDDCX_MONITOR MonitorObject,
	const IDARG_IN_QUERYTARGETMODES2* pInArgs,
	IDARG_OUT_QUERYTARGETMODES* pOutArgs
)
{
	//UNREFERENCED_PARAMETER(MonitorObject);
	auto msg1 = std::format("Querying target modes:\n MonitorObject Handle: {:p}\n TargetModeBufferInputCount: {}",
						   static_cast<void *>(MonitorObject), pInArgs->TargetModeBufferInputCount);
	g_log.Message(Refactoring::LogType::Debug, msg1.c_str());

	vector<IDDCX_TARGET_MODE2> TargetModes(monitorModes.size());

	// Create a set of modes supported for frame processing and scan-out. These are typically not based on the
	// monitor's descriptor and instead are based on the static processing capability of the device. The OS will
	// report the available set of modes for a given output as the intersection of monitor modes with target modes.

	g_log.Message(Refactoring::LogType::Debug, "Creating target modes:");

	for (int i = 0; i < monitorModes.size(); i++)
	{
		CreateTargetMode2(TargetModes[i], monitorModes[i].width, 
			monitorModes[i].height, monitorModes[i].refresh_num, monitorModes[i].refresh_den);
	}

	pOutArgs->TargetModeBufferOutputCount = (UINT)TargetModes.size();

	g_log.Message(Refactoring::LogType::Debug, std::format("Output target modes count: {}", pOutArgs->TargetModeBufferOutputCount).c_str());

	if (pInArgs->TargetModeBufferInputCount >= TargetModes.size())
	{
		copy(TargetModes.begin(), TargetModes.end(), pInArgs->pTargetModes);

		g_log.Message(Refactoring::LogType::Info, "Target modes copied to output buffer:");
		for (int i = 0; i < TargetModes.size(); i++)
		{
			auto msg2 =
				std::format("\n  TargetModeIndex: {}\n   Size: {}\n   ColourFormat: {}", i, TargetModes[i].Size, g_settings.colours.color_format);
			g_log.Message(Refactoring::LogType::Debug, msg2.c_str());
		}
	}
	else
	{
		g_log.Message(Refactoring::LogType::Warning, "Input buffer is too small for target modes.");
	}

	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS VirtualDisplayDriverEvtIddCxAdapterCommitModes2(
	IDDCX_ADAPTER AdapterObject,
	const IDARG_IN_COMMITMODES2* pInArgs
)
{
	UNREFERENCED_PARAMETER(AdapterObject);
	UNREFERENCED_PARAMETER(pInArgs);

	return STATUS_SUCCESS;
}

_Use_decl_annotations_ NTSTATUS VirtualDisplayDriverEvtIddCxMonitorSetGammaRamp(IDDCX_MONITOR MonitorObject, const IDARG_IN_SET_GAMMARAMP *pInArgs)
{
	UNREFERENCED_PARAMETER(MonitorObject);
	UNREFERENCED_PARAMETER(pInArgs);
	// Check if color space processing is enabled
	if (!g_settings.hdr_advanced.color_space.enabled)
	{
		return STATUS_SUCCESS;
	}

	// Quando il percorso interattivo dalla companion app sarà implementato,
	// distinguere enabled (profilo caricato) da auto_configure (applica all'avvio)

	// Virtual display: nessun hardware su cui applicare la gamma ramp.
	// Le caratteristiche colore del monitor sono comunicate via EDID/monitor description.
	return STATUS_SUCCESS;
}

#pragma endregion