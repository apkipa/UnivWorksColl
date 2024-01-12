#pragma once

#include "Logger.hpp"
#include <string_view>

namespace CTinyC {
    enum class TokenType {
        Plus = 1,               // `+`
        Minus,                  // `-`
        Star,                   // `*`
        Divide,                 // `/`
        LChevron,               // `<`
        RChevron,               // `>`
        LessEqual,              // `<=`
        GreaterEqual,           // `>=`
        Equal,                  // `==`
        Not,                    // `!`
        NotEqual,               // `!=`
        Assign,                 // `=`
        Semicolon,              // `;`
        Comma,                  // `,`
        LParenthesis,           // `(`
        RParenthesis,           // `)`
        LSquareBracket,         // `[`
        RSquareBracket,         // `]`
        LCurlyBracket,          // `{`
        RCurlyBracket,          // `}`
        BlockCommentStart,      // `/*`
        BlockCommentEnd,        // `*/`
        LineCommentStart,       // `//`

        Identifier = 10001,
        IntLiteral,
        StringLiteral,

        KwIf = 20001,
        KwElse,
        KwInt,
        KwReturn,
        KwVoid,
        KwWhile,
    };
    struct TokenPosition {
        int line, column;
    };
    struct Token {
        TokenType type;
        std::string str;
        int line, column;

        bool operator==(Token const& other) const {
            return type == other.type;
        }
    };

    std::string_view token_type_to_str(TokenType t);

    struct Lexer {
        Lexer(Logger* logger) : m_logger(logger), m_line{}, m_column{} {}

        void init(std::string_view str) {
            m_str = str;
            m_line = 1;
            m_column = 1;
        }
        std::optional<Token> next_token();
        std::optional<Token> peek_next_token();

        TokenPosition get_current_position() const {
            return { m_line, m_column };
        }

    private:
        bool is_at_end() const {
            return m_str.empty();
        }
        template<typename... Chars>
        bool match_char(Chars... chars) {
            if (is_at_end()) { return false; }
            for (auto ch : { chars... }) {
                if (m_str[0] == ch) {
                    return true;
                }
            }
            return false;
        }
        char advance_char() {
            if (is_at_end()) { return '\0'; }
            auto ch = m_str[0];
            m_str = m_str.substr(1);
            m_column++;
            // Take care of newlines
            if (ch == '\r' && match_char('\n')) {
                m_str = m_str.substr(1);
                ch = '\n';
            }
            if (ch == '\r' || ch == '\n') {
                m_line++;
                m_column = 1;
            }
            return ch;
        }
        char peek_char() {
            if (is_at_end()) { return '\0'; }
            return m_str[0];
        }
        void skip_space() {
            while (!is_at_end()) {
                if (!match_char(' ', '\t', '\r', '\n')) {
                    break;
                }
                advance_char();
            }
        }

        Logger* m_logger;
        std::string_view m_str;
        int m_line, m_column;
    };
}
