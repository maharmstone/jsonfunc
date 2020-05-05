#include <windows.h>
#include <string>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include "git.h"

using json = nlohmann::json;

using namespace std;

string utf16_to_utf8(const wstring_view& ws) {
	int len;
	string s;

	if (ws == L"")
		return "";

	len = WideCharToMultiByte(CP_UTF8, 0, ws.data(), (int)ws.length(), NULL, 0, NULL, NULL);

	if (len == 0)
		return "";

	s = string(len, ' ');

	WideCharToMultiByte(CP_UTF8, 0, ws.data(), (int)ws.length(), (char*)s.c_str(), len, NULL, NULL);

	return s;
}

wstring utf8_to_utf16(const string_view& s) {
	int len;
	wstring wstr;

	if (s == "")
		return L"";

	len = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.length(), NULL, 0);

	if (len == 0)
		return L"";

	wstr = wstring(len, ' ');

	MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.length(), (WCHAR*)wstr.c_str(), len);

	return wstr;
}

static BSTR __inline bstr(const wstring& ws) {
	return SysAllocStringLen(ws.data(), (UINT)ws.length());
}

extern "C" __declspec(dllexport) BSTR JSON_PRETTY(WCHAR* in) {
	wstring ws;

	if (!in)
		return nullptr;

	try {
		auto j = json::parse(utf16_to_utf8(in));

		string s = j.dump(3);

		ws = utf8_to_utf16(s);
	} catch (...) {
		return nullptr;
	}

	return bstr(ws);
}

extern "C" __declspec(dllexport) BSTR JSON_ARRAY(WCHAR* in) {
	json j;

	if (!in)
		return bstr(L"[]");

	try {
		j = json::parse(utf16_to_utf8(in));
	} catch (...) {
		return nullptr;
	}

	if (j.type() != json::value_t::array)
		return nullptr;

	wstring ws;

	try {
		json ret{json::array()};
		string sv;

		for (const auto& el : j) {
			if (sv.empty()) {
				if (!el.empty())
					sv = el.items().begin().key();
			}

			if (!sv.empty() && el.count(sv) > 0)
				ret.emplace_back(el[sv]);
			else
				ret.emplace_back(nullptr);
		}

		string s = ret.dump();

		ws = utf8_to_utf16(s);
	} catch (...) {
		return nullptr;
	}

	return bstr(ws);
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

	return bstr(utf8_to_utf16(s));
}

extern "C" __declspec(dllexport) BSTR STRING_AGG(WCHAR* jsonw, WCHAR* sepw) {
	json j;

	if (!jsonw || !sepw)
		return nullptr;

	string sep = utf16_to_utf8(sepw);

	try {
		j = json::parse(utf16_to_utf8(jsonw));
	} catch (...) {
		return nullptr;
	}

	if (j.type() != json::value_t::array)
		return nullptr;

	wstring ws;

	try {
		string ret, sv;

		for (const auto& el : j) {
			if (sv.empty()) {
				if (!el.empty())
					sv = el.items().begin().key();
			}

			if (el.count(sv) != 0) {
				if (!ret.empty())
					ret += sep;

				ret += el.at(sv);
			}
		}

		ws = utf8_to_utf16(ret);
	} catch (...) {
		return nullptr;
	}

	return bstr(ws);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	return TRUE;
}

