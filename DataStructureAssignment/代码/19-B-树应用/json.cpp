#include "json.h"
#include <optional>
#include <format>

double npow(double x, int p) {
    double result = 1;
    if (p < 0) {
        p = -p;
        x = 1 / x;
    }
    while (p) {
        if (p & 1) {
            result *= x;
        }
        x *= x;
        p >>= 1;
    }
    return result;
}

void append_utf8_codepoint_to_string(std::string &str, uint32_t cp) {
    if (cp < 0x80) {
        str += static_cast<char>(cp);
    }
    else if (cp < 0x800) {
        str += static_cast<char>(0xc0 + (cp >> 6));
        str += static_cast<char>(0x80 + (cp & 0x3f));
    }
    else if (cp < 0x10000) {
        // Also includes U+D800-DFFF
        str += static_cast<char>(0xe0 + (cp >> 12));
        str += static_cast<char>(0x80 + ((cp >> 6) & 0x3f));
        str += static_cast<char>(0x80 + (cp & 0x3f));
    }
    else if (cp < 0x110000) {
        str += static_cast<char>(0xf0 + (cp >> 18));
        str += static_cast<char>(0x80 + ((cp >> 12) & 0x3f));
        str += static_cast<char>(0x80 + ((cp >> 6) & 0x3f));
        str += static_cast<char>(0x80 + (cp & 0x3f));
    }
    else {
        throw std::runtime_error("Invalid Unicode codepoint");
    }
}

void skip_whitespace(std::string_view sv, size_t &rpos) {
    while (rpos < sv.size() && std::isspace(sv[rpos])) {
        rpos++;
    }
}
char look_ahead(std::string_view sv, size_t &rpos) {
    if (rpos == sv.size()) {
        throw std::runtime_error("Unexpected end of stream while parsing JSON");
    }
    return sv[rpos];
}
std::optional<char> try_look_ahead(std::string_view sv, size_t &rpos) {
    if (rpos == sv.size()) {
        return std::nullopt;
    }
    return sv[rpos];
}
char next_char(std::string_view sv, size_t &rpos) {
    char ch = look_ahead(sv, rpos);
    rpos++;
    return ch;
}
std::optional<char> try_next_char(std::string_view sv, size_t &rpos) {
    auto och = try_look_ahead(sv, rpos);
    rpos++;
    return och;
}

// NOTE: Caller must manually skip leading whitespaces
// NOTE: Numbers are implicitly terminated (finishes parsing as soon
//       as an invalid character is read)
double read_number(std::string_view sv, size_t &rpos) {
    double int_num = 0, frac_num = 0;
    double frac_factor = 10;
    int exp_pow = 0;
    bool neg_exp_pow = false;
    size_t cur_rpos = rpos;
    bool neg_sign = false;
    enum {
        Started, InInteger, InPoint, InFraction, InExponentSign, InExponent,
    } cur_state = Started;
    if (look_ahead(sv, cur_rpos) == '-') {
        neg_sign = true;
        cur_rpos++;
    }
    while (true) {
        auto och = try_look_ahead(sv, cur_rpos);
        if (!och) {
            // Found end of stream, stop parsing
            break;
        }
        char ch = *och;
        if (std::isdigit(ch)) {
            int digit = ch - '0';
            switch (cur_state) {
            case Started:
            case InInteger:
                cur_state = InInteger;
                int_num = int_num * 10 + digit;
                break;
            case InPoint:
            case InFraction:
                cur_state = InFraction;
                frac_num += digit / frac_factor;
                frac_factor *= 10;
                break;
            case InExponentSign:
            case InExponent:
                cur_state = InExponent;
                exp_pow = exp_pow * 10 + digit;
                break;
            default:
                throw std::runtime_error("State machine corrupted while parsing number");
            }
        }
        else if (ch == '.') {
            if (cur_state == InInteger) {
                cur_state = InPoint;
            }
            else {
                // Found invalid dot, stop parsing
                break;
            }
        }
        else if (ch == 'e' || ch == 'E') {
            if (cur_state == InInteger || cur_state == InFraction) {
                cur_state = InExponentSign;
                // Hack: Temporarily update read pos
                cur_rpos++;
                ch = look_ahead(sv, cur_rpos);
                cur_rpos--;
                if (ch == '+') {
                    cur_rpos++;
                }
                else if (ch == '-') {
                    neg_exp_pow = true;
                    cur_rpos++;
                }
            }
            else {
                // Found invalid exponent sign, stop parsing
                break;
            }
        }
        else {
            // Found invalid character, stop parsing
            break;
        }
        // Current iteration succeeded, advance pointer
        cur_rpos++;
    }
    // Validate state
    switch (cur_state) {
    case InInteger:
    case InFraction:
    case InExponent:
        break;
    default:
        throw std::runtime_error("Found invalid number while parsing");
    }
    rpos = cur_rpos;
    return (neg_sign ? -1 : 1) * (int_num + frac_num) * npow(10, (neg_exp_pow ? -1 : 1) * exp_pow);
}

// NOTE: Caller must manually skip leading whitespaces
// NOTE: Strings are explicitly terminated (not ending with `"`
//       will cause exceptions to be thrown)
std::string read_string(std::string_view sv, size_t &rpos) {
    size_t cur_rpos = rpos;
    std::string result;
    char ch;
    if (next_char(sv, cur_rpos) != '"') {
        throw std::runtime_error("String did not begin with `\"`");
    }
    while (true) {
        uint32_t unicode_cp;
        auto parse_hex_digit_fn = [](char ch) -> uint32_t {
            if (std::isdigit(ch)) {
                return ch - '0';
            }
            switch (ch) {
            case 'a':   case 'A':   return 10;
            case 'b':   case 'B':   return 11;
            case 'c':   case 'C':   return 12;
            case 'd':   case 'D':   return 13;
            case 'e':   case 'E':   return 14;
            case 'f':   case 'F':   return 15;
            }
            throw std::runtime_error("Found invalid hex digit while parsing string");
        };
        ch = next_char(sv, cur_rpos);
        if (ch == '"') {
            // Found end of string
            break;
        }
        else if (ch == '\\') {
            // Process escape characters
            switch (next_char(sv, cur_rpos)) {
            case '"':       result += '"';      break;
            case '\\':      result += '\\';     break;
            case '/':       result += '/';      break;
            case 'b':       result += '\b';     break;
            case 'f':       result += '\f';     break;
            case 'n':       result += '\n';     break;
            case 'r':       result += '\r';     break;
            case 't':       result += '\t';     break;
            case 'u':
                unicode_cp = 0;
                for (size_t i = 0; i < 4; i++) {
                    unicode_cp = unicode_cp * 16 + parse_hex_digit_fn(next_char(sv, cur_rpos));
                }
                append_utf8_codepoint_to_string(result, unicode_cp);
                break;
            default:
                throw std::runtime_error("Found invalid escape character while parsing string");
            }
        }
        else {
            result += ch;
        }
    }
    rpos = cur_rpos;
    return result;
}

bool read_boolean(std::string_view sv, size_t &rpos) {
    size_t cur_rpos = rpos;
    bool final_value, failed = false;
    switch (next_char(sv, cur_rpos)) {
    case 't':
        final_value = true;
        if (next_char(sv, cur_rpos) != 'r' || next_char(sv, cur_rpos) != 'u' ||
            next_char(sv, cur_rpos) != 'e')
        {
            failed = true;
        }
        break;
    case 'f':
        final_value = false;
        if (next_char(sv, cur_rpos) != 'a' || next_char(sv, cur_rpos) != 'l' ||
            next_char(sv, cur_rpos) != 's' || next_char(sv, cur_rpos) != 'e')
        {
            failed = true;
        }
        break;
    default:
        failed = true;
        break;
    }
    if (failed) {
        throw std::runtime_error("Found invalid boolean while parsing");
    }
    rpos = cur_rpos;
    return final_value;
}

void read_null(std::string_view sv, size_t &rpos) {
    size_t cur_rpos = rpos;
    if (next_char(sv, cur_rpos) != 'n' || next_char(sv, cur_rpos) != 'u' ||
        next_char(sv, cur_rpos) != 'l' || next_char(sv, cur_rpos) != 'l')
    {
        throw std::runtime_error("Found invalid null while parsing");
    }
    rpos = cur_rpos;
}

namespace json {
    struct JsonHelper {
        static JsonObject parse_jobject(std::string_view sv, size_t &rpos) {
            size_t cur_rpos = rpos;
            JsonObject result_jo;
            bool first = true;
            if (next_char(sv, cur_rpos) != '{') {
                throw std::runtime_error("Found invalid start while parsing object");
            }
            skip_whitespace(sv, cur_rpos);
            while (look_ahead(sv, cur_rpos) != '}') {
                if (!first) {
                    if (next_char(sv, cur_rpos) != ',') {
                        throw std::runtime_error("Found invalid separator while parsing object");
                    }
                    skip_whitespace(sv, cur_rpos);
                }
                std::string key = read_string(sv, cur_rpos);
                skip_whitespace(sv, cur_rpos);
                if (next_char(sv, cur_rpos) != ':') {
                    throw std::runtime_error("Found invalid key-value separator while parsing object");
                }
                JsonValue value = parse_jvalue(sv, cur_rpos);
                result_jo[key] = std::move(value);
                first = false;
            }
            cur_rpos++;
            rpos = cur_rpos;
            return result_jo;
        }
        static JsonArray parse_jarray(std::string_view sv, size_t &rpos) {
            size_t cur_rpos = rpos;
            JsonArray result_ja;
            bool first = true;
            if (next_char(sv, cur_rpos) != '[') {
                throw std::runtime_error("Found invalid start while parsing array");
            }
            skip_whitespace(sv, cur_rpos);
            while (look_ahead(sv, cur_rpos) != ']') {
                if (!first) {
                    if (next_char(sv, cur_rpos) != ',') {
                        throw std::runtime_error("Found invalid separator while parsing array");
                    }
                }
                JsonValue value = parse_jvalue(sv, cur_rpos);
                result_ja.push_back(std::move(value));
                first = false;
            }
            cur_rpos++;
            rpos = cur_rpos;
            return result_ja;
        }
        static JsonValue parse_jvalue(std::string_view sv, size_t &rpos) {
            size_t cur_rpos = rpos;
            JsonValue result_value;
            skip_whitespace(sv, cur_rpos);
            switch (look_ahead(sv, cur_rpos)) {
            case '"':       result_value = read_string(sv, cur_rpos);       break;
            case '{':       result_value = parse_jobject(sv, cur_rpos);     break;
            case '[':       result_value = parse_jarray(sv, cur_rpos);      break;
            case 't':
            case 'f':       result_value = read_boolean(sv, cur_rpos);      break;
            case 'n':       read_null(sv, cur_rpos);                        break;
            default:        result_value = read_number(sv, cur_rpos);       break;
            }
            skip_whitespace(sv, cur_rpos);
            rpos = cur_rpos;
            return result_value;
        }

        static std::string to_string(const JsonValue& jv) {
            std::string tmpstr;
            bool first = true;
            switch (jv.m_kind) {
            case JsonValueKind::Null:
                return "null";
            case JsonValueKind::Boolean:
                return jv.get<bool>() ? "true" : "false";
            case JsonValueKind::Number:
                return std::format("{}", jv.get<double>());
            case JsonValueKind::String:
                tmpstr += '"';
                for (auto ch : jv.get<std::string>()) {
                    switch (ch) {
                    case '"':       tmpstr += "\\\"";       break;
                    case '\\':      tmpstr += "\\\\";       break;
                    case '/':       tmpstr += "\\/";        break;
                    case '\b':      tmpstr += "\\b";        break;
                    case '\f':      tmpstr += "\\f";        break;
                    case '\n':      tmpstr += "\\n";        break;
                    case '\r':      tmpstr += "\\r";        break;
                    case '\t':      tmpstr += "\\t";        break;
                    default:        tmpstr += ch;           break;
                    }
                }
                tmpstr += '"';
                return tmpstr;
            case JsonValueKind::Object:
                tmpstr += '{';
                for (const auto& i : jv.get<JsonObject>()) {
                    if (!first) {
                        tmpstr += ',';
                    }
                    tmpstr += to_string(i.first);
                    tmpstr += ':';
                    tmpstr += to_string(i.second);
                    first = false;
                }
                tmpstr += '}';
                return tmpstr;
            case JsonValueKind::Array:
                tmpstr += '[';
                for (const auto& i : jv.get<JsonArray>()) {
                    if (!first) {
                        tmpstr += ',';
                    }
                    tmpstr += to_string(i);
                    first = false;
                }
                tmpstr += ']';
                return tmpstr;
            default:
                throw std::runtime_error("Invalid JsonValue");
            }
        }
    };

    auto JsonArray::erase(iterator pos) noexcept -> iterator { return m_vec.erase(pos); }
    bool JsonArray::operator==(const JsonArray& rhs) const {
        return m_vec == rhs.m_vec;
    }

    JsonValue& JsonObject::operator[](std::string_view sv) {
        auto iter = m_map.find(std::string{ sv });
        if (iter != m_map.end()) {
            return iter->second;
        }
        else {
            return m_map.insert(std::pair{ std::string(sv), JsonValue{} }).first->second;
        }
    }
    bool JsonObject::operator==(const JsonObject& rhs) const {
        return m_map == rhs.m_map;
    }
    void swap(JsonObject& a, JsonObject& b) noexcept {
        using std::swap;
        swap(a.m_map, b.m_map);
    }

    bool JsonValue::try_deserialize_from_utf8(const char* data, size_t len) {
        try {
            size_t rpos = 0;
            JsonValue result = JsonHelper::parse_jvalue({ data, len }, rpos);
            if (rpos != len) {
                throw std::runtime_error("Found unexpected non-whitespace character after value");
            }
            swap(*this, result);
            return true;
        }
        catch (...) {
            return false;
        }
    }
    std::vector<char> JsonValue::serialize_into_utf8(void) const {
        std::string str = JsonHelper::to_string(*this);
        return { str.begin(), str.end() };
    }
}
