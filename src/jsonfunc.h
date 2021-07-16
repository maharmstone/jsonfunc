#pragma once

#include <windows.h>
#include <string>

// jsonfunc.cpp
std::u16string utf8_to_utf16(const std::string_view& s);
std::string utf16_to_utf8(const std::wstring_view& ws);

static BSTR __inline bstr(const std::u16string_view& ws) {
    return SysAllocStringLen((WCHAR*)ws.data(), (UINT)ws.length());
}
