#pragma once

#include <string>
#include <functional>
#include <optional>
#include <vector>

enum class xml_node {
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
	xml_enc_string_view() { }
	xml_enc_string_view(std::string_view sv) : sv(sv) { }

	bool empty() const noexcept {
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
	xml_reader(std::string_view sv) : sv(sv) { }
	bool read();
	enum xml_node node_type() const;
	bool is_empty() const;
	void attributes_loop_raw(const std::function<bool(std::string_view local_name, xml_enc_string_view namespace_uri_raw,
													xml_enc_string_view value_raw)>& func) const;
													std::optional<xml_enc_string_view> get_attribute(std::string_view name, std::string_view ns = "") const;
													xml_enc_string_view namespace_uri_raw() const;
													std::string_view name() const;
													std::string_view local_name() const;
													std::string value() const;
	std::string_view raw() const;

private:
	std::string_view sv, node;
	enum xml_node type;
	bool empty_tag;
	std::vector<ns_list> namespaces;
};
