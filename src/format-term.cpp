#include "jsonfunc.h"
#include <list>
#include <vector>

using namespace std;

typedef struct {
	string_view code;
	string_view text;
} text_bit;

static string xml_escape(const string_view& sv) {
	string s;

	for (const auto& c : sv) {
		if (c == '<')
			s += "&lt;";
		else if (c == '>')
			s += "&gt;";
		else if (c == '&')
			s += "&amp;";
		else
			s += c;
	}

	return s;
}

extern "C" __declspec(dllexport) BSTR TERM2HTML(WCHAR* inw) noexcept {
	if (!inw)
		return nullptr;

	auto in = utf16_to_utf8((char16_t*)inw);

	bool in_code = false;
	size_t last_text_start = 0, code_start = 0;
	text_bit tb;
	list<text_bit> bits;

	for (size_t i = 0; i < in.size(); i++) {
		if (!in_code && in[i] == 0x1b && i < in.size() -1 && in[i+1] == '[') {
			tb.text = in;
			tb.text = tb.text.substr(last_text_start, i - last_text_start);
			bits.push_back(tb);

			code_start = i + 2;

			in_code = true;
			i++;
			continue;
		} else if (in_code && ((in[i] >= 'A' && in[i] <= 'Z') || (in[i] >= 'a' && in[i] <= 'z'))) {
			in_code = false;

			tb.code = in;
			tb.code = tb.code.substr(code_start, i - code_start + 1);
			last_text_start = i + 1;
		}
	}

	auto it = bits.begin();
	while (it != bits.end()) {
		auto it2 = it;

		it2++;

		if (it->text.length() == 0)
			bits.erase(it);

		it = it2;
	}

	string s;

	for (auto& b : bits) {
		string style;

		if (b.code.length() >= 1 && b.code.back() == 'm') {
			size_t st = 0;
			vector<string_view> codes;

			for (size_t i = 0; i < b.code.length() - 1; i++) {
				if (b.code[i] == ';') {
					string_view c;

					c = b.code;
					c = c.substr(st, i - st);
					codes.push_back(c);

					st = i + 1;
				}
			}

			{
				string_view c;

				c = b.code;
				c = c.substr(st, b.code.length() - st - 1);
				codes.push_back(c);
			}

			for (const auto& c : codes) {
				if (c == "0" || c == "")
					style += "font-weight:normal;color:auto;";
				else if (c == "1")
					style += "font-weight:bold;";
				else if (c == "30")
					style += "color:black;";
				else if (c == "31")
					style += "color:red;";
				else if (c == "32")
					style += "color:green;";
				else if (c == "33")
					style += "color:#cccc00;"; // yellow
				else if (c == "34")
					style += "color:blue;";
				else if (c == "35")
					style += "color:magenta;";
				else if (c == "36")
					style += "color:#00cccc;"; // cyan
				else if (c == "37")
					style += "color:white;";
				//else
					//cerr << "Unhandled display code " << c << "." << endl;
			}
		}

		if (style != "") {
			s += "<span style=\"" + style + "\">";
			s += xml_escape(b.text);
			s += "</span>";
		} else
			s += xml_escape(b.text);
	}

	return bstr(utf8_to_utf16(s));
}
