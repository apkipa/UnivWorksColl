#include "pch.h"

#include "Parser.hpp"

#include "public.h"
#include "Executor.hpp"

#define slogsrc(token, ...) logsrc(L"<source>", (token).line, (token).column, __VA_ARGS__)

namespace CTinyC {
    // Marks common parse error (such as syntax error, ...)
    struct parse_error : std::runtime_error {
        using runtime_error::runtime_error;
    };

    std::string escape_string_token(std::string str) {
        std::string result;
        if (str.empty()) {
            throw std::runtime_error("invalid string token");
        }
        // TODO...
    }

    struct ParserCore {
        ParserCore(Logger* logger, Lexer* lexer) : m_logger(logger), m_lexer(lexer) {}

        std::unique_ptr<ASTN> do_parse() try {
            auto decl_list = declaration_list();
            if (m_has_error) {
                throw parse_error("compilation failed");
            }
            return std::make_unique<ASTN_DeclList>(std::move(decl_list));
        }
        catch (parse_error const& e) {
            m_has_error = true;
            return nullptr;
        }
        catch (std::runtime_error const& e) {
            m_has_error = true;
            m_logger->error(std::format(L"internal compiler error: {}", winrt::to_hstring(e.what())));
            throw;
        }

    private:
        std::optional<Token> next_token() {
            return m_lexer->next_token();
        }
        std::optional<Token> look_ahead() {
            return m_lexer->peek_next_token();
        }
        bool is_at_end() {
            return !look_ahead();
        }
        template<typename... Tokens>
        bool matches(Tokens... tokens) {
            auto token = look_ahead();
            if (!token) { return false; }
            for (auto expected : { tokens... }) {
                if (token->type == expected) {
                    return true;
                }
            }
            return false;
        }
        Token consume(TokenType expected_token) {
            auto token = next_token();
            if (!token || token->type != expected_token) {
                // TODO: Better error reporting
                if (!token) {
                    auto pos = m_lexer->get_current_position();
                    m_logger->error(slogsrc(pos, L"unexpected EOF found, expected {}",
                        winrt::to_hstring(token_type_to_str(expected_token))));
                }
                else {
                    m_logger->error(slogsrc(*token, L"unexpected token `{}` found, expected {}",
                        winrt::to_hstring(token->str), winrt::to_hstring(token_type_to_str(expected_token))));
                }
                throw parse_error("unexpected token found");
            }
            return *token;
        }
        template<typename... Types>
        [[noreturn]] void error(Token const& token, std::wformat_string<Types...> fmt, Types&&... types) {
            m_logger->error(slogsrc(token, fmt, std::forward<Types>(types)...));
            throw parse_error("generic error");
        }
        [[noreturn]] void error_expect(std::wstring_view expect_token) {
            auto token = next_token();
            if (!token) {
                auto pos = m_lexer->get_current_position();
                m_logger->error(slogsrc(pos, L"unexpected EOF found, expected {}",
                    expect_token));
            }
            else {
                m_logger->error(slogsrc(*token, L"unexpected token `{}` found, expected {}",
                    winrt::to_hstring(token->str), expect_token));
            }
            throw parse_error("unexpected token found");
        }
        template<typename... Tokens>
        void recover_until(Tokens... tokens) {
            while (auto token = next_token()) {
                for (auto end_token : { tokens... }) {
                    if (token->type == end_token) {
                        return;
                    }
                }
            }
        }

        std::unique_ptr<ASTN_DeclList> program() {
            return std::make_unique<ASTN_DeclList>(declaration_list());
        }
        std::vector<std::unique_ptr<ASTN_Decl>> declaration_list() {
            std::vector<std::unique_ptr<ASTN_Decl>> result;
            result.push_back(declaration());
            while (!is_at_end()) {
                result.push_back(declaration());
            }
            return result;
        }
        std::unique_ptr<ASTN_Decl> declaration() {
            auto ret_type = type_specifier();
            auto id = identifier();
            if (matches(TokenType::LParenthesis)) {
                // Function declaration
                consume(TokenType::LParenthesis);
                std::vector<ASTData_Param> args{};
                try {
                    args = params();
                    consume(TokenType::RParenthesis);
                }
                catch (parse_error const& e) {
                    m_has_error = true;
                    recover_until(TokenType::RParenthesis);
                }
                if (matches(TokenType::Semicolon)) {
                    consume(TokenType::Semicolon);
                    return std::make_unique<ASTN_FuncDecl>(std::move(ret_type), std::move(id), std::move(args), nullptr);
                }
                auto body = compound_stmt();
                return std::make_unique<ASTN_FuncDecl>(std::move(ret_type), std::move(id), std::move(args), std::move(body));
            }
            while (matches(TokenType::LSquareBracket)) {
                consume(TokenType::LSquareBracket);
                ret_type = { ASTData_Type_Array{ expression(), std::make_unique<ASTData_Type>(std::move(ret_type)) } };
                consume(TokenType::RSquareBracket);
            }
            // TODO: init value
            consume(TokenType::Semicolon);
            return std::make_unique<ASTN_VarDecl>(std::move(ret_type), std::move(id));
        }
        //void var_declaration() {
        //    // TODO...
        //}
        //void fun_declaration() {
        //    // TODO...
        //}
        ASTData_Type type_specifier() {
            if (matches(TokenType::KwVoid, TokenType::KwInt)) {
                auto token = *next_token();
                if (token.type == TokenType::KwVoid) {
                    return { ASTData_Type_Void{} };
                }
                else {
                    return { ASTData_Type_Int{} };
                }
            }
            error_expect(L"type-specifier");
        }
        std::vector<ASTData_Param> params() {
            if (matches(TokenType::KwVoid)) {
                auto void_token = *next_token();
                return {};
            }
            return param_list();
        }
        std::vector<ASTData_Param> param_list() {
            std::vector<ASTData_Param> result;
            if (matches(TokenType::RParenthesis)) {
                return result;
            }
            result.push_back(param());
            while (matches(TokenType::Comma)) {
                next_token();
                result.push_back(param());
            }
            return result;
        }
        ASTData_Param param() {
            auto type = type_specifier();
            auto id = identifier();
            bool is_arr{};
            if (matches(TokenType::LSquareBracket)) {
                consume(TokenType::LSquareBracket);
                consume(TokenType::RSquareBracket);
                is_arr = true;
            }
            return { std::move(type), std::move(id), is_arr };
        }
        std::unique_ptr<ASTN_CompoundStmt> compound_stmt() {
            consume(TokenType::LCurlyBracket);
            auto local_decls = local_declarations();
            auto stmts = statement_list();
            consume(TokenType::RCurlyBracket);
            return std::make_unique<ASTN_CompoundStmt>(std::move(local_decls), std::move(stmts));
        }
        std::vector<std::unique_ptr<ASTN_Decl>> local_declarations() {
            std::vector<std::unique_ptr<ASTN_Decl>> decls;
            while (matches(TokenType::KwInt, TokenType::KwVoid)) {
                decls.push_back(declaration());
            }
            return decls;
        }
        std::vector<std::unique_ptr<ASTN_Stmt>> statement_list() {
            std::vector<std::unique_ptr<ASTN_Stmt>> stmts;
            while (!matches(TokenType::RCurlyBracket)) {
                if (is_at_end()) {
                    consume(TokenType::RCurlyBracket);
                }
                stmts.push_back(statement());
            }
            return stmts;
        }
        std::unique_ptr<ASTN_Stmt> statement() try {
            if (matches(TokenType::LCurlyBracket)) {
                return compound_stmt();
            }
            if (matches(TokenType::KwIf)) {
                return selection_stmt();
            }
            if (matches(TokenType::KwWhile)) {
                return iteration_stmt();
            }
            if (matches(TokenType::KwReturn)) {
                return return_stmt();
            }
            return expression_stmt();
        }
        catch (parse_error const& e) {
            // Recover from error
            m_has_error = true;
            while (auto token = next_token()) {
                if (token->type == TokenType::Semicolon || token->type == TokenType::RCurlyBracket) {
                    break;
                }
            }
            return nullptr;
        }
        std::unique_ptr<ASTN_ExprStmt> expression_stmt() {
            auto expr = expression();
            consume(TokenType::Semicolon);
            return std::make_unique<ASTN_ExprStmt>(std::move(expr));
        }
        std::unique_ptr<ASTN_IfStmt> selection_stmt() {
            consume(TokenType::KwIf);
            consume(TokenType::LParenthesis);
            auto cond = expression();
            consume(TokenType::RParenthesis);
            auto stmt = statement();
            std::unique_ptr<ASTN_Stmt> stmt_else{};
            if (matches(TokenType::KwElse)) {
                next_token();
                stmt_else = statement();
            }
            return std::make_unique<ASTN_IfStmt>(std::move(cond), std::move(stmt), std::move(stmt_else));
        }
        std::unique_ptr<ASTN_WhileStmt> iteration_stmt() {
            consume(TokenType::KwWhile);
            consume(TokenType::LParenthesis);
            auto cond = expression();
            consume(TokenType::RParenthesis);
            auto stmt = statement();
            return std::make_unique<ASTN_WhileStmt>(std::move(cond), std::move(stmt));
        }
        std::unique_ptr<ASTN_ReturnStmt> return_stmt() {
            consume(TokenType::KwReturn);
            std::unique_ptr<ASTN_Expr> expr{};
            if (!matches(TokenType::Semicolon)) {
                expr = expression();
            }
            consume(TokenType::Semicolon);
            return std::make_unique<ASTN_ReturnStmt>(std::move(expr));
        }
        std::unique_ptr<ASTN_Expr> expression() {
            // Recursion for right association
            std::unique_ptr<ASTN_Expr> lhs_expr = simple_expression();
            if (matches(TokenType::Assign)) {
                auto op = *next_token();
                auto rhs_expr = expression();
                lhs_expr = std::make_unique<ASTN_BinaryExpr>(std::move(lhs_expr), std::move(rhs_expr), std::move(op));
            }
            return lhs_expr;
        }
        std::unique_ptr<ASTN_IdExpr> var() {
            auto id = identifier();
            std::vector<std::unique_ptr<ASTN_Expr>> arridxs;
            while (matches(TokenType::LSquareBracket)) {
                auto op = *next_token();
                arridxs.push_back(expression());
                consume(TokenType::RSquareBracket);
            }
            return std::make_unique<ASTN_IdExpr>(std::move(id), std::move(arridxs));
        }
        std::unique_ptr<ASTN_Expr> simple_expression() {
            std::unique_ptr<ASTN_Expr> lhs = additive_expression();
            while (matches(TokenType::LessEqual, TokenType::LChevron,
                TokenType::RChevron, TokenType::GreaterEqual,
                TokenType::Equal, TokenType::NotEqual))
            {
                auto op = relop();
                auto rhs = additive_expression();
                lhs = std::make_unique<ASTN_BinaryExpr>(std::move(lhs), std::move(rhs), std::move(op));
            }
            return lhs;
        }
        Token relop() {
            if (matches(TokenType::LessEqual, TokenType::LChevron,
                TokenType::RChevron, TokenType::GreaterEqual,
                TokenType::Equal, TokenType::NotEqual))
            {
                return *next_token();
            }
            error_expect(L"relop");
        }
        std::unique_ptr<ASTN_Expr> additive_expression() {
            std::unique_ptr<ASTN_Expr> lhs = term();
            while (matches(TokenType::Plus, TokenType::Minus))
            {
                auto op = addop();
                auto rhs = term();
                lhs = std::make_unique<ASTN_BinaryExpr>(std::move(lhs), std::move(rhs), std::move(op));
            }
            return lhs;
        }
        Token addop() {
            if (matches(TokenType::Plus, TokenType::Minus)) {
                return *next_token();
            }
            error_expect(L"addop");
        }
        std::unique_ptr<ASTN_Expr> term() {
            std::unique_ptr<ASTN_Expr> lhs = factor();
            while (matches(TokenType::Star, TokenType::Divide)) {
                auto op = mulop();
                auto rhs = factor();
                lhs = std::make_unique<ASTN_BinaryExpr>(std::move(lhs), std::move(rhs), std::move(op));
            }
            return lhs;
        }
        Token mulop() {
            if (matches(TokenType::Star, TokenType::Divide)) {
                return *next_token();
            }
            error_expect(L"mulop");
        }
        std::unique_ptr<ASTN_Expr> factor() {
            if (matches(TokenType::IntLiteral, TokenType::StringLiteral)) {
                return std::make_unique<ASTN_LiteralExpr>(*next_token());
            }
            if (matches(TokenType::LParenthesis)) {
                next_token();
                auto expr = expression();
                consume(TokenType::RParenthesis);
                return expr;
            }
            std::unique_ptr<ASTN_Expr> id_expr = var();
            if (matches(TokenType::LParenthesis)) {
                next_token();
                auto expr = std::make_unique<ASTN_CallExpr>(std::move(id_expr), args());
                consume(TokenType::RParenthesis);
                return expr;
            }
            return id_expr;
        }
        //void call() {
        //    // TODO...
        //}
        std::vector<std::unique_ptr<ASTN_Expr>> args() {
            std::vector<std::unique_ptr<ASTN_Expr>> result;
            if (matches(TokenType::RParenthesis)) {
                // No args
                return result;
            }
            result.push_back(expression());
            while (matches(TokenType::Comma)) {
                next_token();
                result.push_back(expression());
            }
            return result;
        }
        //void arg_list() {
        //    // TODO...
        //}
        Token identifier() {
            return consume(TokenType::Identifier);
        }

        Logger* m_logger;
        Lexer* m_lexer;
        bool m_has_error{};
    };

    std::unique_ptr<ASTN> Parser::parse(Lexer* lexer) {
        ParserCore parser_core(m_logger, lexer);
        return parser_core.do_parse();
    }
}
