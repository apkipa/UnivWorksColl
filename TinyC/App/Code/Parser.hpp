#pragma once

#include "Logger.hpp"
#include "Lexer.hpp"

#include <variant>

namespace CTinyC {
    // ASTN stands for AbstractSyntaxTreeNode
    struct ASTN_Expr;
    struct ASTN_Stmt;
    struct ASTN_Decl;

    struct ASTData_Type;
    struct ASTData_Type_Int {
        bool operator==(ASTData_Type_Int const& other) const { return true; }
    };
    struct ASTData_Type_Void {
        bool operator==(ASTData_Type_Void const& other) const { return true; }
    };
    struct ASTData_Type_Pointer {
        std::unique_ptr<ASTData_Type> inner;
        /*ASTData_Type_Pointer(ASTData_Type_Pointer const& other) {
            inner = std::make_unique<ASTData_Type>(other.inner->t);
        }*/
        bool operator==(ASTData_Type_Pointer const& other) const;
    };
    struct ASTData_Type_Array {
        std::unique_ptr<ASTN_Expr> dimension;
        std::unique_ptr<ASTData_Type> inner;
        /*ASTData_Type_Array(ASTData_Type_Array const& other) {
            dimension = std::make_unique<ASTN_Expr>(other.dimension);
            inner = std::make_unique<ASTData_Type>(other.inner->t);
        }*/
        bool operator==(ASTData_Type_Array const& other) const;
    };
    struct ASTData_Type {
        std::variant<ASTData_Type_Int, ASTData_Type_Void,
            ASTData_Type_Pointer, ASTData_Type_Array> t;
        auto operator<=>(ASTData_Type const& other) const = default;
    };
    struct ASTData_Param {
        ASTData_Type type;
        Token param;
        bool is_arr;
        bool operator==(ASTData_Param const& other) const {
            return std::tie(type, param, is_arr) ==
                std::tie(other.type, other.param, other.is_arr);
        }
    };

    struct ASTN_ExprVisitor;
    struct ASTN_Expr {
        virtual void accept(ASTN_ExprVisitor& visitor) const = 0;
        virtual ~ASTN_Expr() {}
        bool operator==(ASTN_Expr const& other) const {
            // TODO...
            return true;
        }
    };
    struct ASTN_IdExpr;
    struct ASTN_BinaryExpr;
    struct ASTN_UnaryExpr;
    struct ASTN_CallExpr;
    struct ASTN_LiteralExpr;
    struct ASTN_ExprVisitor {
        virtual void visit_id_expr(ASTN_IdExpr const& v) = 0;
        virtual void visit_binary_expr(ASTN_BinaryExpr const& v) = 0;
        virtual void visit_unary_expr(ASTN_UnaryExpr const& v) = 0;
        virtual void visit_call_expr(ASTN_CallExpr const& v) = 0;
        virtual void visit_literal_expr(ASTN_LiteralExpr const& v) = 0;
    };
    struct ASTN_IdExpr : ASTN_Expr {
        ASTN_IdExpr(Token id, std::vector<std::unique_ptr<ASTN_Expr>> arridxs) :
            id(std::move(id)), arridxs(std::move(arridxs)) {}
        void accept(ASTN_ExprVisitor& visitor) const override {
            visitor.visit_id_expr(*this);
        }
        Token id;
        std::vector<std::unique_ptr<ASTN_Expr>> arridxs;
    };
    struct ASTN_BinaryExpr : ASTN_Expr {
        ASTN_BinaryExpr(std::unique_ptr<ASTN_Expr> left, std::unique_ptr<ASTN_Expr> right, Token op) :
            left(std::move(left)), right(std::move(right)), op(std::move(op)) {}
        void accept(ASTN_ExprVisitor& visitor) const override {
            visitor.visit_binary_expr(*this);
        }
        std::unique_ptr<ASTN_Expr> left, right;
        Token op;
    };
    struct ASTN_UnaryExpr : ASTN_Expr {
        ASTN_UnaryExpr(std::unique_ptr<ASTN_Expr> right, Token op) :
            right(std::move(right)), op(std::move(op)) {}
        void accept(ASTN_ExprVisitor& visitor) const override {
            visitor.visit_unary_expr(*this);
        }
        std::unique_ptr<ASTN_Expr> right;
        Token op;
    };
    struct ASTN_CallExpr : ASTN_Expr {
        ASTN_CallExpr(std::unique_ptr<ASTN_Expr> callee, std::vector<std::unique_ptr<ASTN_Expr>> args) :
            callee(std::move(callee)), args(std::move(args)) {}
        void accept(ASTN_ExprVisitor& visitor) const override {
            visitor.visit_call_expr(*this);
        }
        std::unique_ptr<ASTN_Expr> callee;
        std::vector<std::unique_ptr<ASTN_Expr>> args;
    };
    struct ASTN_LiteralExpr : ASTN_Expr {
        ASTN_LiteralExpr(Token value) : value(std::move(value)) {}
        void accept(ASTN_ExprVisitor& visitor) const override {
            visitor.visit_literal_expr(*this);
        }
        Token value;
    };

    struct ASTN_StmtVisitor;
    struct ASTN_Stmt {
        virtual void accept(ASTN_StmtVisitor& visitor) const = 0;
        virtual ~ASTN_Stmt() {}
    };
    struct ASTN_ExprStmt;
    struct ASTN_IfStmt;
    struct ASTN_WhileStmt;
    struct ASTN_ReturnStmt;
    struct ASTN_CompoundStmt;
    struct ASTN_StmtVisitor {
        virtual void visit_expr_stmt(ASTN_ExprStmt const& v) = 0;
        virtual void visit_if_stmt(ASTN_IfStmt const& v) = 0;
        virtual void visit_while_stmt(ASTN_WhileStmt const& v) = 0;
        virtual void visit_return_stmt(ASTN_ReturnStmt const& v) = 0;
        virtual void visit_compound_stmt(ASTN_CompoundStmt const& v) = 0;
    };
    struct ASTN_ExprStmt : ASTN_Stmt {
        ASTN_ExprStmt(std::unique_ptr<ASTN_Expr> expr) : expr(std::move(expr)) {}
        void accept(ASTN_StmtVisitor& visitor) const override {
            visitor.visit_expr_stmt(*this);
        }
        std::unique_ptr<ASTN_Expr> expr;
    };
    struct ASTN_IfStmt : ASTN_Stmt {
        ASTN_IfStmt(std::unique_ptr<ASTN_Expr> cond, std::unique_ptr<ASTN_Stmt> body, std::unique_ptr<ASTN_Stmt> else_body) :
            cond(std::move(cond)), body(std::move(body)), else_body(std::move(else_body)) {}
        void accept(ASTN_StmtVisitor& visitor) const override {
            visitor.visit_if_stmt(*this);
        }
        std::unique_ptr<ASTN_Expr> cond;
        std::unique_ptr<ASTN_Stmt> body;
        std::unique_ptr<ASTN_Stmt> else_body;
    };
    struct ASTN_WhileStmt : ASTN_Stmt {
        ASTN_WhileStmt(std::unique_ptr<ASTN_Expr> cond, std::unique_ptr<ASTN_Stmt> body) :
            cond(std::move(cond)), body(std::move(body)) {}
        void accept(ASTN_StmtVisitor& visitor) const override {
            visitor.visit_while_stmt(*this);
        }
        std::unique_ptr<ASTN_Expr> cond;
        std::unique_ptr<ASTN_Stmt> body;
    };
    struct ASTN_ReturnStmt : ASTN_Stmt {
        ASTN_ReturnStmt(std::unique_ptr<ASTN_Expr> expr) : expr(std::move(expr)) {}
        void accept(ASTN_StmtVisitor& visitor) const override {
            visitor.visit_return_stmt(*this);
        }
        std::unique_ptr<ASTN_Expr> expr;
    };
    struct ASTN_CompoundStmt : ASTN_Stmt {
        ASTN_CompoundStmt(std::vector<std::unique_ptr<ASTN_Decl>> decls, std::vector<std::unique_ptr<ASTN_Stmt>> stmts) :
            decls(std::move(decls)), stmts(std::move(stmts)) {}
        void accept(ASTN_StmtVisitor& visitor) const override {
            visitor.visit_compound_stmt(*this);
        }
        std::vector<std::unique_ptr<ASTN_Decl>> decls;
        std::vector<std::unique_ptr<ASTN_Stmt>> stmts;
    };

    struct ASTN_DeclVisitor;
    struct ASTN_Decl {
        virtual void accept(ASTN_DeclVisitor& visitor) const = 0;
        virtual ~ASTN_Decl() {}
    };
    struct ASTN_VarDecl;
    struct ASTN_FuncDecl;
    struct ASTN_DeclVisitor {
        virtual void visit_var_decl(ASTN_VarDecl const& v) = 0;
        virtual void visit_func_decl(ASTN_FuncDecl const& v) = 0;
    };
    struct ASTN_VarDecl : ASTN_Decl {
        ASTN_VarDecl(ASTData_Type type, Token id) : type(std::move(type)), id(std::move(id)) {}
        void accept(ASTN_DeclVisitor& visitor) const override {
            visitor.visit_var_decl(*this);
        }
        ASTData_Type type;
        Token id;
        //std::unique_ptr<ASTN_Expr> init_value;
    };
    struct ASTN_FuncDecl : ASTN_Decl {
        ASTN_FuncDecl(ASTData_Type ret_type, Token id, std::vector<ASTData_Param> params, std::unique_ptr<ASTN_Stmt> body) :
            ret_type(std::move(ret_type)), id(std::move(id)), params(std::move(params)), body(std::move(body)) {}
        void accept(ASTN_DeclVisitor& visitor) const override {
            visitor.visit_func_decl(*this);
        }
        ASTData_Type ret_type;
        Token id;
        std::vector<ASTData_Param> params;
        std::unique_ptr<ASTN_Stmt> body;
    };

    struct ASTN_Visitor;
    struct ASTN {
        virtual void accept(ASTN_Visitor& visitor) const = 0;
        virtual ~ASTN() {}
    };
    struct ASTN_DeclList;
    struct ASTN_Visitor {
        virtual void visit_decl_list(ASTN_DeclList const& v) = 0;
    };
    struct ASTN_DeclList : ASTN {
        ASTN_DeclList(std::vector<std::unique_ptr<ASTN_Decl>> decls) : decls(std::move(decls)) {}
        void accept(ASTN_Visitor& visitor) const override {
            visitor.visit_decl_list(*this);
        }
        std::vector<std::unique_ptr<ASTN_Decl>> decls;
    };

    struct Parser {
        Parser(Logger* logger) : m_logger(logger) {}

        // NOTE: This method throws exceptions on failure
        std::unique_ptr<ASTN> parse(Lexer* lexer);

    private:
        Logger* m_logger;
    };



    inline bool ASTData_Type_Pointer::operator==(ASTData_Type_Pointer const& other) const {
        return *inner == *other.inner;
    }
    inline bool ASTData_Type_Array::operator==(ASTData_Type_Array const& other) const {
        return std::tie(*dimension, *inner) ==
            std::tie(*other.dimension, *other.inner);
    }
}