#include <windows.h>
#include <string>
#include <stdexcept>
#include "json.h"
#include "git.h"

using namespace std;

string utf16_to_utf8(const wstring_view& ws) {
	int len;
	string s;

	if (ws == L"")
		return "";

	len = WideCharToMultiByte(CP_UTF8, 0, ws.data(), ws.length(), NULL, 0, NULL, NULL);

	if (len == 0)
		return "";

	s = string(len, ' ');

	WideCharToMultiByte(CP_UTF8, 0, ws.data(), ws.length(), (char*)s.c_str(), len, NULL, NULL);

	return s;
}

wstring utf8_to_utf16(const string_view& s) {
	int len;
	wstring wstr;

	if (s == "")
		return L"";

	len = MultiByteToWideChar(CP_UTF8, 0, s.data(), s.length(), NULL, 0);

	if (len == 0)
		return L"";

	wstr = wstring(len, ' ');

	MultiByteToWideChar(CP_UTF8, 0, s.data(), s.length(), (WCHAR*)wstr.c_str(), len);

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
		return SysAllocString(L"[]");

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

extern "C" __declspec(dllexport) BSTR git_file(WCHAR* repodirw, WCHAR* fnw) {
	string repodir = utf16_to_utf8(repodirw);
	string fn = utf16_to_utf8(fnw);
	string s;

	git_libgit2_init();

	try {
		GitRepo repo(repodir);

		GitTree tree(repo, "HEAD");

		GitBlob blob(tree, fn);

		s = blob;
	} catch (...) {
		git_libgit2_shutdown();
		return nullptr;
	}

	git_libgit2_shutdown();

	return SysAllocString(utf8_to_utf16(s).c_str());
}

extern "C" __declspec(dllexport) BSTR STRING_AGG(WCHAR* jsonw, WCHAR* sepw) {
	json j;

	if (!jsonw || !sepw)
		return nullptr;

	string sep = utf16_to_utf8(sepw);

	try {
		j.parse(utf16_to_utf8(jsonw));
	} catch (...) {
		return nullptr;
	}

	if (j.type != json_class_type::array)
		return nullptr;

	string ret, sv;
	const vector<json>& v = j.values();

	for (const auto& el : v) {
		if (sv == "") {
			vector<string> k = el.keys();

			if (!k.empty())
				sv = k[0];
		}

		if (el.count(sv) > 0) {
			if (!ret.empty())
				ret += sep;

			ret += el[sv];
		}
	}

	return SysAllocString(utf8_to_utf16(ret).c_str());
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	return TRUE;
}

