/* json11
 *
 * json11 is a tiny JSON library for C++11, providing JSON parsing and serialization.
 *
 * The core object provided by the library is json11::Json. A Json object represents any JSON
 * value: null, bool, number (int or double), string (std::string), array (std::vector), or
 * object (std::map).
 *
 * Json objects act like values: they can be assigned, copied, moved, compared for equality or
 * order, etc. There are also helper methods Json::dump, to serialize a Json to a string, and
 * Json::parse (static) to parse a std::string as a Json object.
 *
 * Internally, the various types of Json object are represented by the JsonValue class
 * hierarchy.
 *
 * A note on numbers - JSON specifies the syntax of number formatting but not its semantics,
 * so some JSON implementations distinguish between integers and floating-point numbers, while
 * some don't. In json11, we choose the latter. Because some JSON implementations (namely
 * Javascript itself) treat all numbers as the same type, distinguishing the two leads
 * to JSON that will be *silently* changed by a round-trip through those implementations.
 * Dangerous! To avoid that risk, json11 stores all numbers as double internally, but also
 * provides integer helpers.
 *
 * Fortunately, double-precision IEEE754 ('double') can precisely store any integer in the
 * range +/-2^53, which includes every 'int' on most systems. (Timestamps often use int64
 * or long long to avoid the Y2038K problem; a double storing microseconds since some epoch
 * will be exact for +/- 275 years.)
 */

/* Copyright (c) 2013 Dropbox, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <initializer_list>

#ifdef _MSC_VER
    #if _MSC_VER <= 1800 // VS 2013
        #ifndef noexcept
            #define noexcept throw()
        #endif

        #ifndef snprintf
            #define snprintf _snprintf_s
        #endif
    #endif
#endif

namespace json11 {

enum JsonParse {
    STANDARD, COMMENTS
};

class JsonValue;

class Json final {
public:
    // Types
    enum Type {
        NUL, NUMBER, BOOL, STRING, ARRAY, OBJECT
    };

    // Array and object typedefs
    typedef std::vector<Json> array;
    typedef std::map<std::string, Json> object;

    // Constructors for the various types of JSON value.
    Json() noexcept;                // NUL
    Json(std::nullptr_t) noexcept;  // NUL
    Json(double value);             // NUMBER
    Json(int value);                // NUMBER
    Json(bool value);               // BOOL
    Json(const std::string &value); // STRING
    Json(std::string &&value);      // STRING
    Json(const char * value);       // STRING
    Json(const array &values);      // ARRAY
    Json(array &&values);           // ARRAY
    Json(const object &values);     // OBJECT
    Json(object &&values);          // OBJECT

    // Implicit constructor: anything with a to_json() function.
    template <class T, class = decltype(&T::to_json)>
    Json(const T & t) : Json(t.to_json()) {}

    // Implicit constructor: map-like objects (std::map, std::unordered_map, etc)
    template <class M, typename std::enable_if<
        std::is_constructible<std::string, decltype(std::declval<M>().begin()->first)>::value
        && std::is_constructible<Json, decltype(std::declval<M>().begin()->second)>::value,
            int>::type = 0>
    Json(const M & m) : Json(object(m.begin(), m.end())) {}

    // Implicit constructor: vector-like objects (std::list, std::vector, std::set, etc)
    template <class V, typename std::enable_if<
        std::is_constructible<Json, decltype(*std::declval<V>().begin())>::value,
            int>::type = 0>
    Json(const V & v) : Json(array(v.begin(), v.end())) {}

    // This prevents Json(some_pointer) from accidentally producing a bool. Use
    // Json(bool(some_pointer)) if that behavior is desired.
    Json(void *) = delete;

    // Accessors
    Type type() const;

    bool is_null()   const { return type() == NUL; }
    bool is_number() const { return type() == NUMBER; }
    bool is_bool()   const { return type() == BOOL; }
    bool is_string() const { return type() == STRING; }
    bool is_array()  const { return type() == ARRAY; }
    bool is_object() const { return type() == OBJECT; }

    // Return the enclosed value if this is a number, 0 otherwise. Note that json11 does not
    // distinguish between integer and non-integer numbers - number_value() and int_value()
    // can both be applied to a NUMBER-typed object.
    double number_value() const;
    int int_value() const;

    // Return the enclosed value if this is a boolean, false otherwise.
    bool bool_value() const;
    // Return the enclosed string if this is a string, "" otherwise.
    const std::string &string_value() const;
    // Return the enclosed std::vector if this is an array, or an empty vector otherwise.
    const array &array_items() const;
    // Return the enclosed std::map if this is an object, or an empty map otherwise.
    const object &object_items() const;

    // Return a reference to arr[i] if this is an array, Json() otherwise.
    const Json & operator[](size_t i) const;
    // Return a reference to obj[key] if this is an object, Json() otherwise.
    const Json & operator[](const std::string &key) const;

    // Serialize.
    void dump(std::string &out) const;
    std::string dump() const {
        std::string out;
        dump(out);
        return out;
    }

    // Parse. If parse fails, return Json() and assign an error message to err.
    static Json parse(const std::string & in,
                      std::string & err,
                      JsonParse strategy = JsonParse::STANDARD);
    static Json parse(const char * in,
                      std::string & err,
                      JsonParse strategy = JsonParse::STANDARD) {
        if (in) {
            return parse(std::string(in), err, strategy);
        } else {
            err = "null input";
            return nullptr;
        }
    }
    // Parse multiple objects, concatenated or separated by whitespace
    static std::vector<Json> parse_multi(
        const std::string & in,
        std::string::size_type & parser_stop_pos,
        std::string & err,
        JsonParse strategy = JsonParse::STANDARD);

    static inline std::vector<Json> parse_multi(
        const std::string & in,
        std::string & err,
        JsonParse strategy = JsonParse::STANDARD) {
        std::string::size_type parser_stop_pos;
        return parse_multi(in, parser_stop_pos, err, strategy);
    }

    bool operator== (const Json &rhs) const;
    bool operator<  (const Json &rhs) const;
    bool operator!= (const Json &rhs) const { return !(*this == rhs); }
    bool operator<= (const Json &rhs) const { return !(rhs < *this); }
    bool operator>  (const Json &rhs) const { return  (rhs < *this); }
    bool operator>= (const Json &rhs) const { return !(*this < rhs); }

    /* has_shape(types, err)
     *
     * Return true if this is a JSON object and, for each item in types, has a field of
     * the given type. If not, return false and set err to a descriptive message.
     */
    typedef std::initializer_list<std::pair<std::string, Type>> shape;
    bool has_shape(const shape & types, std::string & err) const;

private:
    std::shared_ptr<JsonValue> m_ptr;
};

// Internal class hierarchy - JsonValue objects are not exposed to users of this API.
class JsonValue {
protected:
    friend class Json;
    friend class JsonInt;
    friend class JsonDouble;
    virtual Json::Type type() const = 0;
    virtual bool equals(const JsonValue * other) const = 0;
    virtual bool less(const JsonValue * other) const = 0;
    virtual void dump(std::string &out) const = 0;
    virtual double number_value() const;
    virtual int int_value() const;
    virtual bool bool_value() const;
    virtual const std::string &string_value() const;
    virtual const Json::array &array_items() const;
    virtual const Json &operator[](size_t i) const;
    virtual const Json::object &object_items() const;
    virtual const Json &operator[](const std::string &key) const;
    virtual ~JsonValue() {}
};

} // namespace json11

// -----------------------------------------
// END OF JSON11 LIB
// external helper functions

#include <fstream>
#include <sstream>
#include <glm/glm.hpp>

// -----------------------------------------
// file I/O

inline void write_json_config(const char* filename, const json11::Json& cfg) {
    std::ofstream outcfg(filename);
    outcfg << cfg.dump();
    printf("%s written.\n", filename);
}

inline json11::Json read_json_config(const char* filename,
        const std::vector<std::pair<std::string, std::string>> patches = {}) {
    // read entire file into buffer
	std::ifstream file(filename);
	std::stringstream buffer;
	buffer << file.rdbuf();
	// parse json
	std::string err;
	json11::Json cfg = json11::Json::parse(buffer.str(), err);
	if (err.empty())
		return cfg;
	else
		fprintf(stderr, "Failed to parse config file %s: %s\n", filename, err.c_str());
	return json11::Json();
}

// -----------------------------------------
// typed parsing helpers

inline void json_set_int(const json11::Json& cfg, const char* key, int& value) {
    if (cfg[key].is_number())
        value = cfg[key].int_value();
}
inline void json_set_uint(const json11::Json& cfg, const char* key, uint32_t& value) {
    if (cfg[key].is_number())
        value = uint32_t(cfg[key].int_value());
}
inline void json_set_size(const json11::Json& cfg, const char* key, size_t& value) {
    if (cfg[key].is_number())
        value = size_t(cfg[key].int_value());
}
inline void json_set_float(const json11::Json& cfg, const char* key, float& value) {
    if (cfg[key].is_number())
        value = cfg[key].number_value();
}
inline void json_set_vec2(const json11::Json& cfg, const char* key, glm::vec2& v) {
    if (cfg[key].is_array() && cfg[key].array_items().size() >= 2) {
        v[0] = cfg[key].array_items()[0].number_value();
        v[1] = cfg[key].array_items()[1].number_value();
    }
}
inline void json_set_vec3(const json11::Json& cfg, const char* key, glm::vec3& v) {
    if (cfg[key].is_array() && cfg[key].array_items().size() >= 3) {
        v[0] = cfg[key].array_items()[0].number_value();
        v[1] = cfg[key].array_items()[1].number_value();
        v[2] = cfg[key].array_items()[2].number_value();
    }
}
inline void json_set_vec4(const json11::Json& cfg, const char* key, glm::vec4& v) {
    if (cfg[key].is_array() && cfg[key].array_items().size() >= 4) {
        v[0] = cfg[key].array_items()[0].number_value();
        v[1] = cfg[key].array_items()[1].number_value();
        v[2] = cfg[key].array_items()[2].number_value();
        v[3] = cfg[key].array_items()[3].number_value();
    }
}
inline void json_set_bool(const json11::Json& cfg, const char* key, bool& value) {
    if (cfg[key].is_bool())
        value = cfg[key].bool_value();
}
inline void json_set_string(const json11::Json& cfg, const char* key, std::string& value) {
    if (cfg[key].is_string())
        value = cfg[key].string_value();
}
