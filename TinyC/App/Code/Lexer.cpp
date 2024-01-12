#include "pch.h"

#include "Lexer.hpp"

#include "public.h"

namespace CTinyC {
    std::string_view token_type_to_str(TokenType t) {
        switch (t) {
#define GEN_CASE(type, str) case (TokenType::type): return (str)
            GEN_CASE(Plus, "`+`");
            GEN_CASE(Minus, "`-`");
            GEN_CASE(Star, "`*`");
            GEN_CASE(Divide, "`/`");
            GEN_CASE(LChevron, "`<`");
            GEN_CASE(RChevron, "`>`");
            GEN_CASE(LessEqual, "`<=`");
            GEN_CASE(GreaterEqual, "`>=`");
            GEN_CASE(Equal, "`==`");
            GEN_CASE(Not, "`!`");
            GEN_CASE(NotEqual, "`!=`");
            GEN_CASE(Assign, "`=`");
            GEN_CASE(Semicolon, "`;`");
            GEN_CASE(Comma, "`,`");
            GEN_CASE(LParenthesis, "`(`");
            GEN_CASE(RParenthesis, "`)`");
            GEN_CASE(LSquareBracket, "`[`");
            GEN_CASE(RSquareBracket, "`]`");
            GEN_CASE(LCurlyBracket, "`{`");
            GEN_CASE(RCurlyBracket, "`}`");
            GEN_CASE(BlockCommentStart, "`/*`");
            GEN_CASE(BlockCommentEnd, "`*/`");
            GEN_CASE(LineCommentStart, "`//`");
            GEN_CASE(Identifier, "identifier");
            GEN_CASE(IntLiteral, "int-literal");
            GEN_CASE(StringLiteral, "string-literal");
            GEN_CASE(KwIf, "`if`");
            GEN_CASE(KwElse, "`else`");
            GEN_CASE(KwInt, "`int`");
            GEN_CASE(KwReturn, "`return`");
            GEN_CASE(KwVoid, "`void`");
            GEN_CASE(KwWhile, "`while`");
#undef GEN_CASE
        default:
            return {};
        }
    }

    std::optional<Token> Lexer::next_token() {
        skip_space();
        if (is_at_end()) { return std::nullopt; }
        int last_line = m_line, last_column = m_column;
        auto make_fn = [&](TokenType type, std::string s) {
            return Token{ type, s, last_line, last_column };
        };
        auto ch = advance_char();
        if (ch == '+') { return make_fn(TokenType::Plus, "+"); }
        if (ch == '-') { return make_fn(TokenType::Minus, "-"); }
        if (ch == '*') { return make_fn(TokenType::Star, "*"); }
        //if (ch == '/') { return make_fn(TokenType::Divide, "/"); }
        if (ch == '<') {
            if (match_char('=')) {
                advance_char();
                return make_fn(TokenType::LessEqual, "<=");
            }
            return make_fn(TokenType::LChevron, "<");
        }
        if (ch == '>') {
            if (match_char('=')) {
                advance_char();
                return make_fn(TokenType::GreaterEqual, ">=");
            }
            return make_fn(TokenType::RChevron, ">");
        }
        if (ch == '=') {
            if (match_char('=')) {
                advance_char();
                return make_fn(TokenType::Equal, "==");
            }
            return make_fn(TokenType::Assign, "=");
        }
        if (ch == '!') {
            if (match_char('=')) {
                advance_char();
                return make_fn(TokenType::NotEqual, "!=");
            }
            return make_fn(TokenType::Not, "!");
        }
        if (ch == ';') { return make_fn(TokenType::Semicolon, ";"); }
        if (ch == ',') { return make_fn(TokenType::Comma, ","); }
        if (ch == '(') { return make_fn(TokenType::LParenthesis, "("); }
        if (ch == ')') { return make_fn(TokenType::RParenthesis, ")"); }
        if (ch == '[') { return make_fn(TokenType::LSquareBracket, "["); }
        if (ch == ']') { return make_fn(TokenType::RSquareBracket, "]"); }
        if (ch == '{') { return make_fn(TokenType::LCurlyBracket, "{"); }
        if (ch == '}') { return make_fn(TokenType::RCurlyBracket, "}"); }
        if (ch == '/') {
            if (match_char('*')) {
                // Swallow block comment
                advance_char();
                while (!is_at_end()) {
                    if (advance_char() == '*' && peek_char() == '/') {
                        advance_char();
                        break;
                    }
                }
            }
            else if (match_char('/')) {
                // Swallow line comment
                advance_char();
                while (!is_at_end()) {
                    auto ch = advance_char();
                    if (ch == '\r' || ch == '\n') {
                        break;
                    }
                }
            }
            else {
                return make_fn(TokenType::Divide, "/");
            }
            // We swallowed the comment, so start over
            return next_token();
        }
        if (std::isdigit(ch)) {
            std::string str;
            str += ch;
            while (std::isalnum(peek_char())) {
                str += advance_char();
            }
            return make_fn(TokenType::IntLiteral, str);
        }
        if (ch == '"') {
            // Handle string literal
            std::string str;
            str += ch;
            char last_ch = ch;
            while (!is_at_end()) {
                last_ch = ch;
                ch = advance_char();
                str += ch;
                if (ch == '"' && last_ch != '\\') {
                    return make_fn(TokenType::StringLiteral, str);
                }
            }
            throw std::runtime_error("string literal did not terminate");
        }
        // All remaining characters are considered identifiers
        std::string str;
        std::string_view exclude_chars{ "+-*/<>=!;,()[]{}\"' \t\r\n" };
        str += ch;
        while (!is_at_end()) {
            if (exclude_chars.contains(peek_char())) { break; }
            str += advance_char();
        }
        if (!str.empty()) {
            auto type = TokenType::Identifier;
            if (str == "if") { type = TokenType::KwIf; }
            if (str == "else") { type = TokenType::KwElse; }
            if (str == "int") { type = TokenType::KwInt; }
            if (str == "return") { type = TokenType::KwReturn; }
            if (str == "void") { type = TokenType::KwVoid; }
            if (str == "while") { type = TokenType::KwWhile; }
            return make_fn(type, str);
        }
        m_logger->error(logsrc(L"<source>", last_line, last_column,
            L"unrecognized character \\x{:X} found", ch));
        throw std::runtime_error("found invalid character");
    }
    std::optional<Token> Lexer::peek_next_token() {
        int last_line = m_line, last_column = m_column;
        auto last_str = m_str;
        auto token = next_token();
        std::tie(m_line, m_column, m_str) = std::tuple(last_line, last_column, last_str);
        return token;
    }
}
