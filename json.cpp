#include "json.h"

using namespace std;

json::json(long long l) {
	type = json_class_type::integer;
	intval = l;
}

json::json(double d) {
	type = json_class_type::real;
	doubval = d;
}

json::json(vector<json>& v) {
	type = json_class_type::array;

	for (auto& v2 : v) {
		maplist_values.push_back(v2);
	}
}

json::json(bool b) {
	type = json_class_type::boolean;
	boolval = b;
}

void json::push_back(const json& j) {
	if (type == json_class_type::unknown)
		type = json_class_type::array;
	else if (type != json_class_type::array)
		throw runtime_error("JSON type was not array.");

	maplist_values.push_back(j);
}

string json::to_string(bool pretty) const {
	struct json_object* str = to_struct();

	string s = json_object_to_json_string_ext(str, pretty ? (JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY) : JSON_C_TO_STRING_PLAIN);

	json_object_put(str);

	return s;
}

json& json::operator=(const string& s) {
	type = json_class_type::string;
	strval = s;

	return *this;
}

json& json::operator=(const char* s) {
	type = json_class_type::string;
	strval = s;

	return *this;
}

json& json::operator=(long long l) {
	type = json_class_type::integer;
	intval = l;

	return *this;
}

json& json::operator=(unsigned int i) {
	type = json_class_type::integer;
	intval = i;

	return *this;
}

json& json::operator=(bool b) {
	type = json_class_type::boolean;
	boolval = b;

	return *this;
}

json& json::operator=(vector<json>& v) {
	type = json_class_type::array;

	maplist_values.clear();

	for (auto& v2 : v) {
		maplist_values.push_back(v2);
	}

	return *this;
}

json& json::operator[](const string& n) {
	if (type == json_class_type::unknown)
		type = json_class_type::object;
	else if (type != json_class_type::object)
		throw runtime_error("JSON type was not object.");

	for (unsigned int i = 0; i < maplist_keys.size(); i++) {
		if (maplist_keys[i] == n)
			return maplist_values[i];
	}

	maplist_keys.push_back(n);
	maplist_values.push_back(json());

	return maplist_values[maplist_values.size() - 1];
}

json& json::operator[](const char* n) {
	return operator[](string(n));
}

const json& json::operator[](const string& n) const {
	if (type != json_class_type::object)
		throw runtime_error("JSON type was not object.");

	for (unsigned int i = 0; i < maplist_keys.size(); i++) {
		if (maplist_keys[i] == n)
			return maplist_values[i];
	}

	throw out_of_range("Key " + n + " not found.");
}

const json& json::operator[](const char* s) const {
	return operator[](string(s));
}

bool json::operator==(const string& s) const {
	return operator string() == s;
}

struct json_object* json::to_struct() const {
	switch (type) {
		case json_class_type::array:
		{
			struct json_object* str = json_object_new_array();

			for (int i = maplist_values.size() - 1; i >= 0; i--) {
				json_object_array_put_idx(str, i, maplist_values[i].to_struct());
			}

			return str;
		}

		case json_class_type::string:
			return json_object_new_string_len(strval.c_str(), strval.size());

		case json_class_type::object:
		{
			struct json_object* str = json_object_new_object();

			for (unsigned int i = 0; i < maplist_keys.size(); i++) {
				json_object_object_add(str, maplist_keys[i].c_str(), maplist_values[i].to_struct());
			}

			return str;
		}

		case json_class_type::integer:
			return json_object_new_int64(intval);

		case json_class_type::real:
			return json_object_new_double(doubval);

		case json_class_type::boolean:
			return json_object_new_boolean(boolval);

		default:
			return nullptr;
	}
}

json::json(json_object* obj) {
	switch (json_object_get_type(obj)) {
		case json_type_null:
			type = json_class_type::null;
		break;

		case json_type_boolean:
			type = json_class_type::boolean;
			boolval = json_object_get_boolean(obj);
		break;

		case json_type_double:
			type = json_class_type::real;
			doubval = json_object_get_double(obj);
		break;

		case json_type_int:
			type = json_class_type::integer;
			intval = json_object_get_int64(obj);
		break;

		case json_type_string:
			type = json_class_type::string;
			strval = json_object_get_string(obj);
		break;

		case json_type_array:
		{
			unsigned int len = json_object_array_length(obj);

			type = json_class_type::array;
			maplist_values.clear();
			maplist_values.reserve(len);

			for (unsigned int i = 0; i < len; i++) {
				maplist_values.push_back(json_object_array_get_idx(obj, i));
			}

			break;
		}

		case json_type_object:
		{
			lh_table* lht = json_object_get_object(obj);

			if (!lht)
				break;

			lh_entry* lhe = lht->head;

			type = json_class_type::object;
			maplist_keys.clear();
			maplist_values.clear();

			while (lhe) {
				maplist_keys.push_back((char*)lhe->k);
				maplist_values.push_back((json_object*)lhe->v);

				lhe = lhe->next;
			}
		}
	}
}

void json::parse(const string& s) {
	json_object* obj = json_tokener_parse(s.c_str());

	if (!obj)
		throw runtime_error("JSON parsing error.");

	*this = json(obj);

	json_object_put(obj);
}

unsigned int json::count(const string& n) const {
	if (type != json_class_type::object)
		return 0;

	for (const auto& k : maplist_keys) {
		if (k == n)
			return 1;
	}

	return 0;
}

json::operator string() const {
	switch (type) {
		case json_class_type::string:
			return strval;

		case json_class_type::integer:
			return ::to_string(intval);

		case json_class_type::boolean:
			return boolval ? "true" : "false";

		case json_class_type::real:
			return ::to_string(doubval);

		default:
			return "";
	}
}

json::operator signed long long() const {
	switch (type) {
		case json_class_type::string:
			return stoll(strval);

		case json_class_type::integer:
			return intval;

		case json_class_type::boolean:
			return boolval ? 1 : 0;

		case json_class_type::real:
			return (signed long long)doubval;

		default:
			return 0;
	}
}

json::operator int() const {
	return (int)operator signed long long();
}

json::operator unsigned int() const {
	return (unsigned int)operator signed long long();
}

json::operator bool() const {
	switch (type) {
		case json_class_type::integer:
			return intval != 0;

		case json_class_type::boolean:
			return boolval;

		default:
			return false;
	}
}

json::operator double() const {
	switch (type) {
		case json_class_type::string:
			return stod(strval);

		case json_class_type::integer:
			return (double)intval;

		case json_class_type::boolean:
			return boolval ? 1.0 : 0.0;

		case json_class_type::real:
			return doubval;

		default:
			return 0.0;
	}
}

vector<string> json::keys() const {
	if (type != json_class_type::object)
		return {};

	return maplist_keys;
}

void json::obj_reserve(unsigned int l) {
	type = json_class_type::object;

	maplist_keys.reserve(l);
	maplist_values.reserve(l);
}

void json::arr_reserve(unsigned int l) {
	type = json_class_type::array;

	maplist_values.reserve(l);
}

json::json(const initializer_list<string>& ls) {
	type = json_class_type::object;
	maplist_keys.reserve(ls.size() >> 1);
	maplist_values.reserve(ls.size() >> 1);

	if (ls.size() % 2 == 1)
		throw runtime_error("Initialization list for json must have an even number of items.");

	bool odd = true;
	for (const auto& l : ls) {
		if (odd)
			maplist_keys.push_back(l);
		else
			maplist_values.push_back(l);

		odd = !odd;
	}
}

void json::emplace(const string& n, const json& j) {
	// FIXME - should check if key already defined

	maplist_keys.emplace_back(n);
	maplist_values.emplace_back(j);
}

void json::erase(const string& n) {
	if (type != json_class_type::object)
		return;

	for (unsigned int i = 0; i < maplist_keys.size(); i++) {
		if (maplist_keys[i] == n) {
			maplist_keys[i] = maplist_keys.back();
			maplist_keys.pop_back();
			maplist_values[i] = maplist_values.back();
			maplist_values.pop_back();
			return;
		}
	}
}
