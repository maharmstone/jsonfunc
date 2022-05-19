#include "xml.h"
#include <charconv>
#include <stdexcept>

using namespace std;

optional<xml_enc_string_view> xml_reader::get_attribute(string_view name, string_view ns) const {
	if (type != xml_node::element)
		return nullopt;

	optional<xml_enc_string_view> xesv;

	attributes_loop_raw([&](string_view local_name, string_view, xml_enc_string_view namespace_uri_raw,
							xml_enc_string_view value_raw) {
		if (local_name == name && namespace_uri_raw.cmp(ns)) {
			xesv = value_raw;
			return false;
		}

		return true;
	});

	return xesv;
}

xml_enc_string_view xml_reader::namespace_uri_raw() const {
	auto tag = name();
	auto colon = tag.find_first_of(':');
	string_view prefix;

	if (colon != string::npos)
		prefix = tag.substr(0, colon);

	for (auto it = namespaces.rbegin(); it != namespaces.rend(); it++) {
		for (const auto& v : *it) {
			if (v.first == prefix)
				return v.second;
		}
	}

	return {};
}

string_view xml_reader::local_name() const {
	if (type != xml_node::element && type != xml_node::end_element)
		return "";

	auto tag = name();
	auto pos = tag.find_first_of(':');

	if (pos == string::npos)
		return tag;
	else
		return tag.substr(pos + 1);
}

string xml_reader::value() const {
	switch (type) {
		case xml_node::text:
			return xml_enc_string_view{node}.decode();

		case xml_node::cdata:
			return string{node.substr(9, node.length() - 12)};

		default:
			return {};
	}
}

static string esc_char(string_view s) {
	uint32_t c = 0;
	from_chars_result fcr;

	if (s.starts_with("x"))
		fcr = from_chars(s.data() + 1, s.data() + s.length(), c, 16);
	else
		fcr = from_chars(s.data(), s.data() + s.length(), c);

	if (c == 0 || c > 0x10ffff)
		return "";

	if (c < 0x80)
		return string{(char)c, 1};
	else if (c < 0x800) {
		char t[2];

		t[0] = (char)(0xc0 | (c >> 6));
		t[1] = (char)(0x80 | (c & 0x3f));

		return string{string_view(t, 2)};
	} else if (c < 0x10000) {
		char t[3];

		t[0] = (char)(0xe0 | (c >> 12));
		t[1] = (char)(0x80 | ((c >> 6) & 0x3f));
		t[2] = (char)(0x80 | (c & 0x3f));

		return string{string_view(t, 3)};
	} else {
		char t[4];

		t[0] = (char)(0xf0 | (c >> 18));
		t[1] = (char)(0x80 | ((c >> 12) & 0x3f));
		t[2] = (char)(0x80 | ((c >> 6) & 0x3f));
		t[3] = (char)(0x80 | (c & 0x3f));

		return string{string_view(t, 4)};
	}
}

string xml_enc_string_view::decode() const {
	auto v = sv;
	string s;

	s.reserve(v.length());

	while (!v.empty()) {
		if (v.front() == '&') {
			v.remove_prefix(1);

			if (v.starts_with("amp;")) {
				s += "&";
				v.remove_prefix(4);
			} else if (v.starts_with("lt;")) {
				s += "<";
				v.remove_prefix(3);
			} else if (v.starts_with("gt;")) {
				s += ">";
				v.remove_prefix(3);
			} else if (v.starts_with("quot;")) {
				s += "\"";
				v.remove_prefix(5);
			} else if (v.starts_with("apos;")) {
				s += "'";
				v.remove_prefix(5);
			} else if (v.starts_with("#")) {
				string_view bit;

				v.remove_prefix(1);

				auto sc = v.find_first_of(';');
				if (sc == string::npos) {
					bit = v;
					v = "";
				} else {
					bit = v.substr(0, sc);
					v.remove_prefix(sc + 1);
				}

				s += esc_char(bit);
			} else
				s += "&";
		} else {
			s += v.front();
			v.remove_prefix(1);
		}
	}

	return s;
}

bool xml_enc_string_view::cmp(string_view str) const {
	for (auto c : sv) {
		if (c == '&')
			return decode() == str;
	}

	return sv == str;
}
