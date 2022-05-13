#include "jsonfunc.h"
#include <stdexcept>
#include <sstream>
#include <nlohmann/json.hpp>
#include "git.h"
#include "xml.h"

using json = nlohmann::json;

using namespace std;

string utf16_to_utf8(u16string_view ws) {
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

u16string utf8_to_utf16(string_view s) {
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

extern "C" __declspec(dllexport) BSTR JSON_PRETTY(WCHAR* in) noexcept {
	u16string ws;

	if (!in)
		return nullptr;

	try {
		auto j = json::parse(utf16_to_utf8((char16_t*)in));

		string s = j.dump(3);

		ws = utf8_to_utf16(s);
	} catch (...) {
		return nullptr;
	}

	return bstr(ws);
}

extern "C" __declspec(dllexport) BSTR JSON_ARRAY(WCHAR* in) noexcept {
	json j;

	if (!in)
		return bstr(u"[]");

	try {
		j = json::parse(utf16_to_utf8((char16_t*)in));
	} catch (...) {
		return nullptr;
	}

	if (j.type() != json::value_t::array)
		return nullptr;

	u16string ws;

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

extern "C" __declspec(dllexport) BSTR git_file(WCHAR* repodirw, WCHAR* fnw) noexcept {
	u16string ws;

	git_libgit2_init();

	try {
		auto repodir = utf16_to_utf8((char16_t*)repodirw);
		auto fn = utf16_to_utf8((char16_t*)fnw);
		string s;

		GitRepo repo(repodir);

		GitTree tree(repo, "HEAD");

		GitBlob blob(tree, fn);

		s = blob;

        ws = utf8_to_utf16(s);
	} catch (...) {
		git_libgit2_shutdown();
		return nullptr;
	}

	git_libgit2_shutdown();

	return bstr(ws);
}

extern "C" __declspec(dllexport) BSTR STRING_AGG(WCHAR* jsonw, WCHAR* sepw) noexcept {
	json j;

	if (!jsonw || !sepw)
		return nullptr;

	auto sep = utf16_to_utf8((char16_t*)sepw);

	try {
		j = json::parse(utf16_to_utf8((char16_t*)jsonw));
	} catch (...) {
		return nullptr;
	}

	if (j.type() != json::value_t::array)
		return nullptr;

	u16string ws;

	try {
		stringstream ret;
		bool first = true;
		string sv;

		for (const auto& el : j) {
			if (sv.empty()) {
				if (!el.empty())
					sv = el.items().begin().key();
			}

			if (el.count(sv) != 0) {
				if (!first)
					ret << sep;

				ret << el.at(sv);
				first = false;
			}
		}

		ws = utf8_to_utf16(ret.str());
	} catch (...) {
		return nullptr;
	}

	return bstr(ws);
}

static constexpr string xml_pretty2(string_view inu) {
	xml_reader r(inu);
	string s;
	string prefix;
	bool needs_newline = false;
	vector<int> has_text;

	s.reserve(inu.length());

	while (r.read()) {
		switch (r.node_type()) {
			case xml_node::whitespace:
				// skip
				break;

			case xml_node::element:
				if (needs_newline) {
					s += "\n";
					needs_newline = false;
				}

				if (has_text.empty() || !has_text.back())
					s += prefix;

				s += r.raw();

				if (!r.is_empty()) {
					prefix += "    ";
					needs_newline = true;
					has_text.push_back(0);
				} else
					s += "\n";

				break;

			case xml_node::end_element:
				if (needs_newline) {
					s += "\n";
					needs_newline = false;
				}

				prefix.erase(prefix.length() - 4);

				if (!has_text.back())
					s += prefix;
				s += r.raw();
				has_text.pop_back();

				if (has_text.empty() || !has_text.back())
					s += "\n";
				break;

			case xml_node::text:
				needs_newline = false;
				has_text.back() = 1;
				s += r.raw();
				break;

			case xml_node::processing_instruction:
				s += r.raw();
				s += "\n";
				break;

			default:
				if (needs_newline) {
					s += "\n";
					needs_newline = false;
				}

				s += r.raw();
				break;
		}
	}

	return s;
}

static_assert(xml_pretty2("<a><b /><c att=\"value\">text</c><d><e></e></d><f>hel<b>lo wor</b>ld</f><g><h/>text</g></a>") == R"(<a>
    <b />
    <c att="value">text</c>
    <d>
        <e>
        </e>
    </d>
    <f>hel<b>lo wor</b>ld</f>
    <g>
        <h/>
text</g>
</a>
)");
static_assert(xml_pretty2("<a></a>") == "<a>\n</a>\n");
static_assert(xml_pretty2("<?xml version=\"1.0\"?><a></a>") == "<?xml version=\"1.0\"?>\n<a>\n</a>\n");
static_assert(xml_pretty2("<a>\n\n<b> a </b>\t\n</a>") == "<a>\n    <b> a </b>\n</a>\n");
static_assert(xml_pretty2("<a>\n\n<b>  \t  </b>\t\n</a>") == "<a>\n    <b>\n    </b>\n</a>\n");

extern "C" __declspec(dllexport) BSTR XML_PRETTY(WCHAR* in) noexcept {
	u16string ws;

	if (!in)
		return nullptr;

	try {
		auto inu = utf16_to_utf8((char16_t*)in);

		ws = utf8_to_utf16(xml_pretty2(inu));
	} catch (...) {
		return nullptr;
	}

	return bstr(ws);
}
