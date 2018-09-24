#pragma once

#include <json-c/json.h>
#include <vector>
#include <string>

using namespace std;

enum class json_class_type {
	unknown,
	array,
	string,
	object,
	integer,
	real,
	boolean,
	null
};

template<class T> class nullable;

class json {
public:
	json() {
	}

	json(const string& s) : type(json_class_type::string), strval(s) { }
	json(long long l);
	json(double d);
	json(vector<json>& v);
	json(bool b);
	json(const initializer_list<string>& ls);

	template<typename T>
	json(const nullable<T>& s) {
		if (s.is_null())
			type = json_class_type::null;
		else
			*this = (T)s;
	}

	template<typename T>
	json(const T& t) {
		*this = t.to_json();
	}

	json(const vector<json>& v) {
		type = json_class_type::array;
		arr_reserve(v.size());

		for (const auto& j : v) {
			push_back(j);
		}
	}

	void push_back(const json& j);
	string to_string(bool pretty = false) const;
	json& operator=(const string& s);
	json& operator=(const char* s);
	json& operator=(long long l);
	json& operator=(unsigned int i);
	json& operator=(bool b);
	json& operator=(vector<json>& v);
	json& operator[](const string& n);
	json& operator[](const char* n);
	const json& operator[](const string& n) const;
	const json& operator[](const char* s) const;
	bool operator==(const string& s) const;
	operator string() const;
	operator signed long long() const;
	operator int() const;
	operator unsigned int() const;
	operator bool() const;
	operator double() const;
	void parse(const string& s);
	unsigned int count(const string& n) const;
	vector<string> keys() const;
	void obj_reserve(unsigned int l);
	void arr_reserve(unsigned int l);
	void emplace(const string& n, const json& j);
	void erase(const string& n);

	json_class_type type = json_class_type::unknown;

private:
	json(json_object* obj);

	struct json_object* to_struct() const;

	vector<string> maplist_keys;
	vector<json> maplist_values;
	string strval;
	long long intval;
	double doubval;
	bool boolval;
};
