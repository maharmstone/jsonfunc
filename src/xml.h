#pragma once

#include <string>
#include <functional>
#include <optional>
#include <vector>
#include <stdexcept>

enum class xml_node {
	none,
	text,
	whitespace,
	element,
	end_element,
	processing_instruction,
	comment,
	cdata
};

class xml_enc_string_view {
public:
	constexpr xml_enc_string_view() { }
	constexpr xml_enc_string_view(std::string_view sv) : sv(sv) { }

	constexpr bool empty() const noexcept {
		return sv.empty();
	}

	std::string decode() const;
	bool cmp(std::string_view str) const;

private:
	std::string_view sv;
};

using ns_list = std::vector<std::pair<std::string_view, xml_enc_string_view>>;

class xml_reader {
public:
	constexpr xml_reader(std::string_view sv) : sv(sv) { }

	constexpr bool read() {
		if (sv.empty())
			return false;

		// FIXME - DOCTYPE (<!DOCTYPE greeting SYSTEM "hello.dtd">, <!DOCTYPE greeting [ <!ELEMENT greeting (#PCDATA)> ]>)

		if (type == xml_node::element && empty_tag)
			namespaces.pop_back();

		if (sv.front() != '<') { // text
			auto pos = sv.find_first_of('<');

			if (pos == std::string::npos) {
				node = sv;
				sv = "";
			} else {
				node = sv.substr(0, pos);
				sv = sv.substr(pos);
			}

			type = xml_node::whitespace;

			for (auto c : sv) {
				if (!is_whitespace(c)) {
					type = xml_node::text;
					break;
				}
			}
		} else {
			if (sv.starts_with("<?")) {
				auto pos = sv.find("?>");

				if (pos == std::string::npos) {
					node = sv;
					sv = "";
				} else {
					node = sv.substr(0, pos + 2);
					sv = sv.substr(pos + 2);
				}

				type = xml_node::processing_instruction;
			} else if (sv.starts_with("</")) {
				auto pos = sv.find_first_of('>');

				if (pos == std::string::npos) {
					node = sv;
					sv = "";
				} else {
					node = sv.substr(0, pos + 1);
					sv = sv.substr(pos + 1);
				}

				type = xml_node::end_element;
				namespaces.pop_back();
			} else if (sv.starts_with("<!--")) {
				auto pos = sv.find("-->");

				if (pos == std::string::npos)
					throw std::runtime_error("Malformed comment.");

				node = sv.substr(0, pos + 3);
				sv = sv.substr(pos + 3);

				type = xml_node::comment;
			} else if (sv.starts_with("<![CDATA[")) {
				auto pos = sv.find("]]>");

				if (pos == std::string::npos)
					throw std::runtime_error("Malformed CDATA.");

				node = sv.substr(0, pos + 3);
				sv = sv.substr(pos + 3);

				type = xml_node::cdata;
			} else {
				auto pos = sv.find_first_of('>');

				if (pos == std::string::npos) {
					node = sv;
					sv = "";
				} else {
					node = sv.substr(0, pos + 1);
					sv = sv.substr(pos + 1);
				}

				type = xml_node::element;
				ns_list ns;

				parse_attributes(node, [&](std::string_view name, xml_enc_string_view value) {
					if (name.starts_with("xmlns:"))
						ns.emplace_back(name.substr(6), value);
					else if (name == "xmlns")
						ns.emplace_back("", value);

					return true;
				});

				namespaces.push_back(ns);

				empty_tag = node.ends_with("/>");
			}
		}

		return true;
	}

	constexpr enum xml_node node_type() const {
		return type;
	}

	constexpr bool is_empty() const {
		return type == xml_node::element && empty_tag;
	}

	void attributes_loop_raw(const std::function<bool(std::string_view local_name, xml_enc_string_view namespace_uri_raw,
													xml_enc_string_view value_raw)>& func) const;
													std::optional<xml_enc_string_view> get_attribute(std::string_view name, std::string_view ns = "") const;
													xml_enc_string_view namespace_uri_raw() const;
													std::string_view name() const;
													std::string_view local_name() const;
													std::string value() const;

	constexpr std::string_view raw() const {
		return node;
	}

private:
	static constexpr bool __inline is_whitespace(char c) {
		return c == ' ' || c == '\t' || c == '\r' || c == '\n';
	}

	template<typename T>
	static constexpr void parse_attributes(std::string_view node, T func) {
		auto s = node.substr(1, node.length() - 2);

		if (!s.empty() && s.back() == '/') {
			s.remove_suffix(1);
		}

		while (!s.empty() && !is_whitespace(s.front())) {
			s.remove_prefix(1);
		}

		while (!s.empty() && is_whitespace(s.front())) {
			s.remove_prefix(1);
		}

		while (!s.empty()) {
			auto av = s;

			auto eq = av.find_first_of('=');
			std::string_view n, v;

			if (eq == std::string::npos) {
				n = av;
				v = "";
			} else {
				n = av.substr(0, eq);
				v = av.substr(eq + 1);

				while (!v.empty() && is_whitespace(v.front())) {
					v = v.substr(1);
				}

				if (v.length() >= 2 && (v.front() == '"' || v.front() == '\'')) {
					auto c = v.front();

					v.remove_prefix(1);

					auto end = v.find_first_of(c);

					if (end != std::string::npos) {
						v = v.substr(0, end);
						av = av.substr(0, v.data() + v.length() - av.data() + 1);
					} else {
						for (size_t i = 0; i < av.length(); i++) {
							if (is_whitespace(av[i])) {
								av = av.substr(0, i);
								break;
							}
						}
					}
				}
			}

			while (!n.empty() && is_whitespace(n.back())) {
				n.remove_suffix(1);
			}

			if (!func(n, v))
				return;

			s.remove_prefix(av.length());

			while (!s.empty() && is_whitespace(s.front())) {
				s.remove_prefix(1);
			}
		}
	}

	std::string_view sv, node;
	enum xml_node type = xml_node::none;
	bool empty_tag;
	std::vector<ns_list> namespaces;
};
