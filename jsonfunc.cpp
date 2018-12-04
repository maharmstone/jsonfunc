#include <windows.h>
#include <string>
#include "json.h"

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
	json j;

	if (!in)
		return nullptr;

	try {
		j.parse(utf16_to_utf8(in));
	} catch (...) {
		return nullptr;
	}

	string s = j.to_string(true);

	return SysAllocString(utf8_to_utf16(s).c_str());
}

extern "C" __declspec(dllexport) BSTR JSON_ARRAY(WCHAR* in) {
	json j;

	if (!in)
		return nullptr;

	try {
		j.parse(utf16_to_utf8(in));
	} catch (...) {
		return nullptr;
	}

	if (j.type != json_class_type::array)
		return nullptr;

	json ret;
	string sv;
	const vector<json>& v = j.values();

	ret.arr_reserve(0);
	for (const auto& el : v) {
		if (sv == "") {
			vector<string> k = el.keys();

			if (!k.empty())
				sv = k[0];
		}

		if (sv != "" && el.count(sv) > 0)
			ret.push_back(el[sv]);
		else
			ret.push_back(nullptr);
	}

	string s = ret.to_string(false);

	return SysAllocString(utf8_to_utf16(s).c_str());
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	return TRUE;
}

