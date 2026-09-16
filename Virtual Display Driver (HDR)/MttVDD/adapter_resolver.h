#pragma once

#include <algorithm>
#include <cstdint>
#include <devguid.h>
#include <devpkey.h>
#include <devpropdef.h>
#include <dxgi.h> // For IDXGIAdapter, IDXGIFactory1
#include <optional>
#include <setupapi.h>
#include <string>
#include <vector>
#include <wrl/client.h> // For ComPtr

using namespace Microsoft::WRL;

// DEVPKEY_Device_Luid: {60b193cb-5276-4d0f-96fc-f173ab17af69}, 2
// Define it ourselves to avoid SDK/WDK header differences where DEVPKEY_Device_Luid may not be declared.
static const DEVPROPKEY DEVPKEY_Device_Luid_Custom = {{0x60b193cb, 0x5276, 0x4d0f, {0x96, 0xfc, 0xf1, 0x73, 0xab, 0xad, 0x3e, 0xc6}}, 2};

namespace Refactoring
{
struct GPUInfo
{
	std::string name;			  // GPU name
	ComPtr<IDXGIAdapter> adapter; // COM pointer to the adapter
	DXGI_ADAPTER_DESC desc;		  // Adapter description
};

struct ResolvedAdapter
{
	bool hasTargetAdapter = false;	// Indicates if a target adapter is selected
	LUID adapterLuid{};			// Adapter's unique identifier (LUID)
	std::string target_name{}; // Target adapter name
};

bool CompareGPUs(const GPUInfo &a, const GPUInfo &b)
{
	return a.desc.DedicatedVideoMemory > b.desc.DedicatedVideoMemory;
}

// Get a enumerate list of available GPUs
std::vector<GPUInfo> getAvailableGPUs()
{
	std::vector<GPUInfo> gpus; // Vector to hold all GPU's information

	ComPtr<IDXGIFactory1> factory;
	if (!SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
	{
		return gpus;
	}

	// Enumerate all adapters (GPUs)
	for (UINT i = 0;; i++)
	{
		ComPtr<IDXGIAdapter> adapter;
		if (!SUCCEEDED(factory->EnumAdapters(i, &adapter)))
		{
			break;
		}

		DXGI_ADAPTER_DESC desc;

		if (!SUCCEEDED(adapter->GetDesc(&desc)))
		{
			continue;
		}

		// Add the adapter information to the list
		GPUInfo info{Refactoring::WStringToString(desc.Description), adapter, desc};
		gpus.push_back(info);
	}

	return gpus;
}

// Resolve an adapter LUID from a PCI bus number by enumerating display devices (SetupAPI).
// Returns nullopt if no match is found or if the system doesn't expose the LUID property.
std::optional<LUID> ResolveAdapterLuidFromPciBus(uint32_t targetBusIndex)
{
	HDEVINFO devInfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY, nullptr, nullptr, DIGCF_PRESENT);
	if (devInfo == INVALID_HANDLE_VALUE)
	{
		return std::nullopt;
	}

	SP_DEVINFO_DATA devData = {};
	devData.cbSize = sizeof(devData);

	std::optional<LUID> result = std::nullopt;

	for (DWORD i = 0; SetupDiEnumDeviceInfo(devInfo, i, &devData); ++i)
	{
		DWORD currentBus = 0;
		if (!SetupDiGetDeviceRegistryPropertyW(devInfo, &devData, SPDRP_BUSNUMBER, nullptr, reinterpret_cast<PBYTE>(&currentBus), sizeof(currentBus),
											   nullptr))
		{
			continue;
		}

		if (static_cast<uint32_t>(currentBus) != targetBusIndex)
		{
			continue;
		}

		// DEVPKEY_Device_Luid is exposed as a UINT64 on Windows; convert into LUID.
		DEVPROPTYPE propType = 0;
		ULONG propSize = 0;
		ULONGLONG luid64 = 0;

		if (!SetupDiGetDevicePropertyW(devInfo, &devData, &DEVPKEY_Device_Luid_Custom, &propType, reinterpret_cast<PBYTE>(&luid64), sizeof(luid64),
									   &propSize, 0))
		{
			continue;
		}

		if (propType != DEVPROP_TYPE_UINT64 || propSize != sizeof(luid64))
		{
			continue;
		}

		LUID luid{};
		luid.LowPart = static_cast<DWORD>(luid64 & 0xFFFFFFFFull);
		luid.HighPart = static_cast<LONG>((luid64 >> 32) & 0xFFFFFFFFull);
		result = luid;
		break;
	}

	SetupDiDestroyDeviceInfoList(devInfo);
	return result;
}

bool findAndSetAdapter(const std::string &adapterSpec, ResolvedAdapter &adp_options)
{
	// If user provides "name,bus", use bus to resolve LUID (more deterministic on multi-GPU setups).
	const size_t comma = adapterSpec.find(L',');
	if (comma != std::string::npos)
	{
		const std::string namePart = adapterSpec.substr(0, comma);
		std::string busPart = adapterSpec.substr(comma + 1);
		// Trim whitespace in bus part
		busPart.erase(remove_if(busPart.begin(), busPart.end(), iswspace), busPart.end());

		char *end = nullptr;
		const unsigned long busUl = strtoul(busPart.c_str(), &end, 10);
		const bool parsedOk = (end != nullptr) && (*end == '\0') && (end != busPart.c_str());
		if (parsedOk && busUl <= 0xFFFFFFFFul)
		{
			if (auto luidOpt = ResolveAdapterLuidFromPciBus(static_cast<uint32_t>(busUl)); luidOpt.has_value())
			{
				adp_options.adapterLuid = luidOpt.value();
				adp_options.hasTargetAdapter = true;
				return true;
			}
		}

		// Fall through to name matching using the name portion.
		return findAndSetAdapter(namePart, adp_options);
	}

	auto gpus = getAvailableGPUs();

	// Iterate through all available GPUs
	for (const auto &gpu : gpus)
	{
		if (std::strcmp(gpu.name.c_str(), adapterSpec.c_str()) == 0)
		{
			adp_options.adapterLuid = gpu.desc.AdapterLuid; // Set the adapter LUID
			adp_options.hasTargetAdapter = true;			// Indicate that a target adapter is selected
			return true;
		}
	}

	adp_options.hasTargetAdapter = false; // Indicate that no target adapter is selected
	return false;
}

std::string selectBestGPU()
{
	auto gpus = getAvailableGPUs();
	if (gpus.empty())
	{
		return ""; // Error check for headless / vm
	}

	// Sort GPUs by dedicated video memory in descending order
	std::sort(gpus.begin(), gpus.end(), CompareGPUs);
	auto bestGPU = gpus.front(); // Get the GPU with the most memory

	return bestGPU.name;
}
}
