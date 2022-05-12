#pragma once

#include <windows.h>
#include <string>

// jsonfunc.cpp
std::u16string utf8_to_utf16(std::string_view s);
std::string utf16_to_utf8(std::u16string_view ws);

static BSTR __inline bstr(std::u16string_view ws) noexcept {
    return SysAllocStringLen((WCHAR*)ws.data(), (UINT)ws.length());
}
