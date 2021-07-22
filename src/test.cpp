#include <windows.h>
#include <string>
#include <iostream>

extern "C" __declspec(dllexport) BSTR XML_PRETTY(WCHAR* in) noexcept;

using namespace std;

static string utf16_to_utf8(const u16string_view& ws) {
	int len;
	string s;

	if (ws.empty())
		return "";

	len = WideCharToMultiByte(CP_UTF8, 0, (WCHAR*)ws.data(), (int)ws.length(), NULL, 0, NULL, NULL);

	if (len == 0)
		return "";

	s = string(len, ' ');

	WideCharToMultiByte(CP_UTF8, 0, (WCHAR*)ws.data(), (int)ws.length(), (char*)s.c_str(), len, NULL, NULL);

	return s;
}

static u16string utf8_to_utf16(const string_view& s) {
	int len;
	u16string wstr;

	if (s == "")
		return u"";

	len = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.length(), NULL, 0);

	if (len == 0)
		return u"";

	wstr = u16string(len, ' ');

	MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.length(), (WCHAR*)wstr.c_str(), len);

	return wstr;
}

static void test(const string_view& s) {
	auto w = utf8_to_utf16(s);

	cout << s << " -> ";

	auto bstr = XML_PRETTY((WCHAR*)w.c_str());

	if (!bstr) {
		cout << "NULL" << endl;
		return;
	}

	auto s2 = utf16_to_utf8(u16string_view{(char16_t*)bstr, SysStringLen(bstr)});

	SysFreeString(bstr);

	cout << s2 << endl;
}

int main() {
	test("<a><b /><c att=\"value\">text</c><d><e></e></d><f>hel<b>lo wor</b>ld</f><g><h/>text</g></a>");

	return 0;
}
