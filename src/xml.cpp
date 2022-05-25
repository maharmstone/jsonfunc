#include <windows.h>
#include "jsonfunc.h"
#include "xml.h"

using namespace std;

static constexpr string reescape_att(string_view sv) {
	string s;

	while (!sv.empty()) {
		auto c = sv.front();

		sv.remove_prefix(1);

		if (c == '&') {
			auto sc = sv.find_first_of(';');

			if (sc == string::npos)
				throw runtime_error("Unterminated entity.");

			auto ent = sv.substr(0, sc);

			if (ent == "apos")
				s += "'";
			else
				s += "&" + string(ent) + ";";

			// FIXME - decode decimal or hex references?

			sv = sv.substr(ent.size() + 1);
		} else if (c == '\"')
			s += "&quot;";
		else
			s += c;
	}

	return s;
}

static constexpr string xml_pretty2(string_view inu) {
	string s;
	string prefix;
	bool needs_newline = false;
	vector<int> has_text;

	if (inu.size() >= 3 && (uint8_t)inu[0] == 0xef && (uint8_t)inu[1] == 0xbb && (uint8_t)inu[2] == 0xbf) // BOM
		inu = inu.substr(3);

	xml_reader r(inu);
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

				s += "<";
				s += r.name();

				r.attributes_loop_raw([&](string_view local_name, string_view prefix, xml_enc_string_view, xml_enc_string_view value_raw) -> bool {
					s += " ";

					if (!prefix.empty()) {
						s += prefix;
						s += ":";
					}

					s += local_name;
					s += "=\"";
					s += reescape_att(value_raw.raw());
					s += "\"";

					return true;
				});

				if (r.is_empty())
					s += " /";

				s += ">";

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
			case xml_node::cdata:
				needs_newline = false;

				if (!has_text.empty())
					has_text.back() = 1;

				s += r.raw();
				break;

			case xml_node::processing_instruction:
				s += r.raw();
				s += "\n";
				break;

			case xml_node::comment:
				if (needs_newline) {
					s += "\n";
					needs_newline = false;
				}

				s += prefix;
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
        <h />
text</g>
</a>
)");
static_assert(xml_pretty2("<a></a>") == "<a>\n</a>\n");
static_assert(xml_pretty2("<?xml version=\"1.0\"?><a></a>") == "<?xml version=\"1.0\"?>\n<a>\n</a>\n");
static_assert(xml_pretty2("<a>\n\n<b> a </b>\t\n</a>") == "<a>\n    <b> a </b>\n</a>\n");
static_assert(xml_pretty2("<a>\n\n<b>  \t  </b>\t\n</a>") == "<a>\n    <b>\n    </b>\n</a>\n");
static_assert(xml_pretty2("test<a>\n\n<b>  \t  </b>\t\n</a>") == "test<a>\n    <b>\n    </b>\n</a>\n");
static_assert(xml_pretty2("\xef\xbb\xbf<a>\n\n<b>  \t  </b>\t\n</a>") == "<a>\n    <b>\n    </b>\n</a>\n"); // BOM
static_assert(xml_pretty2("<a><!-- comment --><b></b></a>") == "<a>\n    <!-- comment -->\n    <b>\n    </b>\n</a>\n");
static_assert(xml_pretty2("<a><!-- comment 1 --><b><!-- comment 2 --></b></a>") == "<a>\n    <!-- comment 1 -->\n    <b>\n        <!-- comment 2 -->\n    </b>\n</a>\n");
static_assert(xml_pretty2("a") == "a");
static_assert(xml_pretty2("<c att=\"value\"></c>") == "<c att=\"value\">\n</c>\n");
static_assert(xml_pretty2("<c ns:att=\"value\"></c>") == "<c ns:att=\"value\">\n</c>\n");
static_assert(xml_pretty2("<ns:c att=\"value\"></ns:c>") == "<ns:c att=\"value\">\n</ns:c>\n");
static_assert(xml_pretty2("<a/>") == "<a />\n");
static_assert(xml_pretty2("<a />") == "<a />\n");
static_assert(xml_pretty2("<a att1='foo' att2=\"bar\"/>") == "<a att1=\"foo\" att2=\"bar\" />\n");
static_assert(xml_pretty2("<a att1='&apos;\"' att2=\"'&quot;\"/>") == "<a att1=\"'&quot;\" att2=\"'&quot;\" />\n");
static_assert(xml_pretty2("<a><c><![CDATA[foo]]></c></a>") == "<a>\n    <c><![CDATA[foo]]></c>\n</a>\n");

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
