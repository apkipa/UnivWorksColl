#pragma once

#include <variant>
#include <vector>
#include "hashmap.h"
#include <string_view>

namespace json {
    class JsonValue;
    class JsonObject;
    class JsonArray;

    namespace details {
        template<typename T>
        struct dependent_false_type : std::false_type {};
    }

    enum class JsonValueKind {
        Null = 0,
        Boolean,
        Array,
        Number,
        String,
        Object,
    };

    class JsonArray {
    public:
        using value_type = JsonValue;
        using Container = std::vector<value_type>;

        using allocator_type = typename Container::allocator_type;
        using size_type = typename Container::size_type;
        using difference_type = typename Container::difference_type;
        using reference = typename Container::reference;
        using const_reference = typename Container::const_reference;
        using pointer = typename Container::pointer;
        using const_pointer = typename Container::const_pointer;
        using iterator = typename Container::iterator;
        using const_iterator = typename Container::const_iterator;
        using reverse_iterator = typename Container::reverse_iterator;
        using const_reverse_iterator = typename Container::const_reverse_iterator;

        JsonArray() : m_vec() {}
        ~JsonArray() = default;
        JsonArray(const JsonArray& other) : m_vec(other.m_vec) {}
        JsonArray(JsonArray&& other) noexcept : JsonArray() { swap(*this, other); }
        JsonArray& operator=(JsonArray other) noexcept { swap(*this, other); return *this; }

        // TODO: Add more functions from std::vector
        reference at(size_type pos) { return m_vec.at(pos); }
        const_reference at(size_type pos) const { return m_vec.at(pos); }
        reference operator[](size_type pos) noexcept { return m_vec[pos]; }
        const_reference operator[](size_type pos) const noexcept { return m_vec[pos]; }
        reference front() noexcept { return m_vec.front(); }
        const_reference front() const noexcept { return m_vec.front(); }
        reference back() noexcept { return m_vec.back(); }
        const_reference back() const noexcept { return m_vec.back(); }
        value_type* data() noexcept { return m_vec.data(); }
        const value_type* data() const noexcept { return m_vec.data(); }
        iterator begin() noexcept { return m_vec.begin(); }
        const_iterator begin() const noexcept { return m_vec.begin(); }
        const_iterator cbegin() const noexcept { return m_vec.cbegin(); }
        iterator end() noexcept { return m_vec.end(); }
        const_iterator end() const noexcept { return m_vec.end(); }
        const_iterator cend() const noexcept { return m_vec.cend(); }
        reverse_iterator rbegin() noexcept { return m_vec.rbegin(); }
        const_reverse_iterator rbegin() const noexcept { return m_vec.rbegin(); }
        const_reverse_iterator crbegin() const noexcept { return m_vec.crbegin(); }
        reverse_iterator rend() noexcept { return m_vec.rend(); }
        const_reverse_iterator rend() const noexcept { return m_vec.rend(); }
        const_reverse_iterator crend() const noexcept { return m_vec.crend(); }
        bool empty() const noexcept { return m_vec.empty(); }
        size_type size() const noexcept { return m_vec.size(); }
        size_type max_size() const noexcept { return m_vec.max_size(); }
        void reserve(size_type new_cap) { m_vec.reserve(new_cap); }
        size_type capacity() const noexcept { return m_vec.capacity(); }
        void shrink_to_fit() { m_vec.shrink_to_fit(); }
        void clear() noexcept { m_vec.clear(); }
        // SAFETY: JsonValue is designed to be nothrow-move-assignable
        iterator erase(iterator pos) noexcept;
        void push_back(const value_type& value) { m_vec.push_back(value); }
        void push_back(value_type&& value) { m_vec.push_back(std::move(value)); }

        bool operator==(const JsonArray& rhs) const;
        bool operator!=(const JsonArray& rhs) const {
            return !operator==(rhs);
        }

        friend void swap(JsonArray& a, JsonArray& b) noexcept {
            using std::swap;
            swap(a.m_vec, b.m_vec);
        }

        friend struct JsonHelper;
    private:
        Container m_vec;
    };

    class JsonObject {
    public:
        using key_type = std::string;
        using mapped_type = JsonValue;
        using Container = hashmap::HashMap<key_type, mapped_type>;

        using value_type = typename Container::value_type;
        using allocator_type = typename Container::allocator_type;
        using size_type = typename Container::size_type;
        using difference_type = typename Container::difference_type;
        using pointer = typename Container::pointer;
        using const_pointer = typename Container::const_pointer;
        using reference = value_type&;
        using const_reference = const value_type&;
        using iterator = typename Container::iterator;
        using const_iterator = typename Container::const_iterator;

        JsonObject() : m_map() {}
        ~JsonObject() = default;
        JsonObject(const JsonObject& other) : m_map(other.m_map) {}
        JsonObject(JsonObject&& other) noexcept : JsonObject() { swap(*this, other); }
        JsonObject& operator=(JsonObject other) noexcept { swap(*this, other); return *this; }

        JsonValue& operator[](std::string_view sv);

        // TODO: Add more functions from std::map / std::unordered_map
        // TODO: Improve performance by not constructing temporary objects
        //       (require support from HashMap)
        mapped_type& at(std::string_view key) { return m_map.at(std::string{ key }); }
        const mapped_type& at(std::string_view key) const { return m_map.at(std::string{ key }); }
        iterator begin() noexcept { return m_map.begin(); }
        const_iterator begin() const noexcept { return m_map.begin(); }
        const_iterator cbegin() const noexcept { return m_map.cbegin(); }
        iterator end() noexcept { return m_map.end(); }
        const_iterator end() const noexcept { return m_map.end(); }
        const_iterator cend() const noexcept { return m_map.cend(); }
        bool empty() const noexcept { return m_map.empty(); }
        size_type size() const noexcept { return m_map.size(); }
        size_type max_size() const noexcept { return m_map.max_size(); }
        void clear() noexcept { m_map.clear(); }
        iterator erase(iterator pos) noexcept { return m_map.erase(pos); }
        size_type count(std::string_view key) const { return m_map.count(std::string{ key }); }
        const_iterator find(std::string_view key) const { return m_map.find(std::string{ key }); }
        // NOTE: Not re-exporting this method from the C++20 one
        bool contains(std::string_view key) const { return m_map.count(std::string{ key }) > 0; }

        bool operator==(const JsonObject& rhs) const;
        bool operator!=(const JsonObject& rhs) const {
            return !operator==(rhs);
        }

        friend void swap(JsonObject& a, JsonObject& b) noexcept;

        friend struct JsonHelper;
    private:
        Container m_map;
    };

    class JsonValue {
    public:
        JsonValue() : m_kind(JsonValueKind::Null), m_var(std::monostate{}) {}
        JsonValue(std::nullptr_t) : m_kind(JsonValueKind::Null), m_var(std::monostate{}) {}
        JsonValue(bool v) : m_kind(JsonValueKind::Boolean), m_var(v) {}
        JsonValue(const JsonArray& v) : m_kind(JsonValueKind::Array), m_var(v) {}
        JsonValue(JsonArray&& v) : m_kind(JsonValueKind::Array), m_var(std::move(v)) {}
        //JsonValue(double v) : m_kind(JsonValueKind::Number), m_var(v) {}
        template<typename T, std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>, int> = 0>
        JsonValue(T v) : m_kind(JsonValueKind::Number), m_var(static_cast<double>(v)) {}
        JsonValue(const char* v) : m_kind(JsonValueKind::String), m_var(std::string{ v }) {}
        JsonValue(std::string_view v) : m_kind(JsonValueKind::String), m_var(std::string{ v }) {}
        JsonValue(const std::string& v) : JsonValue(std::string_view{ v }) {}
        JsonValue(std::string&& v) : m_kind(JsonValueKind::String), m_var(std::move(v)) {}
        JsonValue(const JsonObject& v) : m_kind(JsonValueKind::Object), m_var(v) {}
        JsonValue(JsonObject&& v) : m_kind(JsonValueKind::Object), m_var(std::move(v)) {}
        ~JsonValue() = default;
        JsonValue(const JsonValue& other) : m_kind(other.m_kind), m_var(other.m_var) {}
        JsonValue(JsonValue&& other) noexcept : JsonValue() { swap(*this, other); }
        JsonValue& operator=(JsonValue other) noexcept { swap(*this, other); return *this; }

        bool try_deserialize_from_utf8(const char* data, size_t len);
        bool try_deserialize_from_utf8(const std::vector<char>& data) {
            return try_deserialize_from_utf8(data.data(), data.size());
        }
        std::vector<char> serialize_into_utf8(void) const;

        bool is_null(void) const { return m_kind == JsonValueKind::Null; }
        bool is_bool(void) const { return m_kind == JsonValueKind::Boolean; }
        bool is_array(void) const { return m_kind == JsonValueKind::Array; }
        bool is_number(void) const { return m_kind == JsonValueKind::Number; }
        bool is_string(void) const { return m_kind == JsonValueKind::String; }
        bool is_object(void) const { return m_kind == JsonValueKind::Object; }

        // TODO: Add proxy support for integral types ?
        template<typename T>
        T& get(void) {
            return std::get<T>(m_var);
        }
        template<typename T>
        const T& get(void) const {
            return std::get<T>(m_var);
        }
        JsonValue& operator[](size_t idx) {
            return this->get<JsonArray>()[idx];
        }
        const JsonValue& operator[](size_t idx) const {
            return this->get<JsonArray>()[idx];
        }
        JsonValue& operator[](std::string_view sv) {
            return this->get<JsonObject>()[sv];
        }
        JsonValue& at(size_t idx) {
            return this->get<JsonArray>().at(idx);
        }
        const JsonValue& at(size_t idx) const {
            return this->get<JsonArray>().at(idx);
        }
        JsonValue& at(std::string_view sv) {
            return this->get<JsonObject>().at(sv);
        }
        const JsonValue& at(std::string_view sv) const {
            return this->get<JsonObject>().at(sv);
        }
        template<typename T>
        T get_value(void) const {
            if constexpr ((std::is_integral_v<T> || std::is_enum_v<T>) && !std::is_same_v<T, bool>) {
                return static_cast<T>(std::get<double>(m_var));
            }
            else {
                return std::get<T>(m_var);
            }
        }
        template<typename T>
        void set_value(const T& v) {
            // NOTE: bool is specialized below, so no special checking is required here
            constexpr bool valid_number = std::is_arithmetic_v<T>;
            constexpr bool valid_string = std::is_convertible_v<T, std::string>;
            static_assert(valid_number || valid_string, "Invalid set_value type for JsonValue");
            if constexpr (valid_number) {
                m_var = static_cast<double>(v);
                m_kind = JsonValueKind::Number;
            }
            else {  // if constexpr (valid_string)
                m_var = std::string{ v };
                m_kind = JsonValueKind::String;
            }
        }
        template<>
        void set_value(const bool& v) {
            m_var = v;
            m_kind = JsonValueKind::Boolean;
        }
        template<>
        void set_value(const JsonArray& v) {
            m_var = v;
            m_kind = JsonValueKind::Array;
        }
        template<>
        void set_value(const JsonObject& v) {
            m_var = v;
            m_kind = JsonValueKind::Object;
        }
        template<>
        void set_value(const std::nullptr_t&) {
            m_var = std::monostate{};
            m_kind = JsonValueKind::Null;
        }
        void set_value(JsonArray&& v) {
            m_var = std::move(v);
            m_kind = JsonValueKind::Array;
        }
        void set_value(JsonObject&& v) {
            m_var = std::move(v);
            m_kind = JsonValueKind::Object;
        }
        void set_value(std::string&& v) {
            m_var = std::move(v);
            m_kind = JsonValueKind::String;
        }

        bool operator==(const JsonValue& rhs) const {
            return m_kind == rhs.m_kind && m_var == rhs.m_var;
        }
        bool operator!=(const JsonValue& rhs) const {
            return !operator==(rhs);
        }

        friend void swap(JsonValue& a, JsonValue& b) noexcept {
            using std::swap;
            swap(a.m_kind, b.m_kind);
            swap(a.m_var, b.m_var);
        }

        friend struct JsonHelper;
    private:
        JsonValueKind m_kind;
        std::variant<std::monostate, bool, JsonArray, double, std::string, JsonObject> m_var;
    };
}
