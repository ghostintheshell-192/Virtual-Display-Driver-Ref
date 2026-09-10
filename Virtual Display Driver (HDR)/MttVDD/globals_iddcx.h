#pragma once
#include <IddCx.h>

namespace Refactoring
{

struct CursorSettingsIDDCX
{
	IDDCX_XOR_CURSOR_SUPPORT xor_cursor_support_level = IDDCX_XOR_CURSOR_SUPPORT_FULL;
};

struct ColourSettingsIDDCX
{
	IDDCX_BITS_PER_COMPONENT SDR_COLOR = IDDCX_BITS_PER_COMPONENT_8;
	IDDCX_BITS_PER_COMPONENT HDR_COLOR = IDDCX_BITS_PER_COMPONENT_10;
};
} // namespace Refactoring
