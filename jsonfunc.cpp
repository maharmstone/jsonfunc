#include <windows.h>
#include <string>

using namespace std;

static string utf16_to_utf8(const wstring& ws) {
	int len;
	string s;

	if (ws == L"")
		return "";

	len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), ws.length(), NULL, 0, NULL, NULL);

	if (len == 0)
		return "";

	s = string(len, ' ');

	WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), ws.length(), (char*)s.c_str(), len, NULL, NULL);

	return s;
}

static wstring utf8_to_utf16(const string& s) {
	int len;
	wstring wstr;

	if (s == "")
		return L"";

	len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), s.length(), NULL, 0);

	if (len == 0)
		return L"";

	wstr = wstring(len, ' ');

	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), s.length(), (WCHAR*)wstr.c_str(), len);

	return wstr;
}

extern "C" __declspec(dllexport) BSTR JSON_PRETTY(WCHAR* in) {
	string s = u8"Lémon curry? ("s + utf16_to_utf8(in) + ")";

	return SysAllocString(utf8_to_utf16(s).c_str());
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	return TRUE;
}

