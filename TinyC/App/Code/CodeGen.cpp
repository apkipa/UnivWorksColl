#include "pch.h"

#include "public.h"

#include "CodeGen.hpp"
#include "Executor.hpp"

#include <ranges>

#define slogsrc(token, ...) logsrc(L"<source>", (token).line, (token).column, __VA_ARGS__)

const uint32_t PENDING_FIXUP = 0xffffffff;

namespace CTinyC {
    struct BlockFrame;
    struct FuncEntry {
        std::string name;
        ASTData_Type const* ret_type;
        std::vector<ASTData_Param> const* params;
        int code_offset;
        BlockFrame* associated_frame{};
    };
    struct IdEntry {
        std::string name;
        ASTData_Type const* type;
        int offset;
        bool is_param_arr{};
    };
    struct BlockFrame {
        BlockFrame* parent{};
        std::list<IdEntry> ids;
        int cur_sp{};
        FuncEntry* func_ctx{};
    };

    static ASTData_Type g_type_int{ ASTData_Type_Int{} };
    static ASTData_Type g_type_void{ ASTData_Type_Void{} };

    struct CodeGenAstVisitor : ASTN_Visitor, ASTN_DeclVisitor, ASTN_ExprVisitor, ASTN_StmtVisitor {
        CodeGenAstVisitor(Logger* logger, int start_offset) : m_logger(logger), m_start_offset(start_offset) {
            m_frames.push_back({});

            m_bytes.reserve(1024 * 64);
        }

        void start(ASTN const& root_node) {
            // Add builtin functions
            {
                m_funcs.push_back({ "input", &g_type_int, nullptr, (int)size(m_bytes) });
                append_byte(ByteCodeType::DuplicateDword);
                append_byte(ByteCodeType::PopDword);
                append_byte(ByteCodeType::PopDword);
                append_byte(ByteCodeType::SysCall);
                append_dword(3);
                append_byte(ByteCodeType::AdjustStackRefConst);
                append_dword(-4);
                append_byte(ByteCodeType::Ret);
                m_funcs.push_back({ "output", &g_type_void, nullptr, (int)size(m_bytes) });
                append_byte(ByteCodeType::PushStackRef);
                macro_add_imm(8);
                append_byte(ByteCodeType::ReadRefDword);
                append_byte(ByteCodeType::SysCall);
                append_dword(4);
                append_byte(ByteCodeType::Ret);
            }

            root_node.accept(*this);

            // Write metadata
            for (auto const& func_info : m_funcs) {
                if (func_info.code_offset < 0) { continue; }
                m_code_meta.func_meta.push_back({
                    func_info.name, (size_t)func_info.code_offset });
            }
        }

        std::vector<uint8_t> m_bytes;
        CodeMetadata m_code_meta;

    private:
        IdEntry* frame_find_id(BlockFrame& frame, std::string_view name) {
            auto it = std::ranges::find(frame.ids, name, [](IdEntry const& v) { return v.name; });
            if (it == end(frame.ids)) {
                return nullptr;
            }
            return &*it;
        }
        FuncEntry* global_find_func(std::string_view name) {
            auto it = std::ranges::find(m_funcs, name, [](FuncEntry const& v) { return v.name; });
            if (it == end(m_funcs)) {
                return nullptr;
            }
            return &*it;
        }
        uint32_t get_cur_code_pos() const {
            return static_cast<uint32_t>(size(m_bytes) + m_start_offset);
        }
        int32_t evaluate_constant_expr(ASTN_Expr const& expr) {
            struct ConstExprVisitor : ASTN_ExprVisitor {
                ConstExprVisitor() {}

                void visit_id_expr(ASTN_IdExpr const& v) override {
                    throw std::runtime_error("not a constant expression");
                }
                void visit_binary_expr(ASTN_BinaryExpr const& v) override {
                    int32_t v0, v1, v2;
                    v.left->accept(*this);
                    v.right->accept(*this);
                    v2 = values.back(); values.pop_back();
                    v1 = values.back(); values.pop_back();
                    switch (v.op.type) {
                    case TokenType::Assign:
                        throw std::runtime_error("not a constant expression");
                    case TokenType::LessEqual:
                        v0 = v1 <= v2;
                        break;
                    case TokenType::GreaterEqual:
                        v0 = v1 >= v2;
                        break;
                    case TokenType::LChevron:
                        v0 = v1 < v2;
                        break;
                    case TokenType::RChevron:
                        v0 = v1 > v2;
                        break;
                    case TokenType::Equal:
                        v0 = v1 == v2;
                        break;
                    case TokenType::NotEqual:
                        v0 = v1 != v2;
                        break;
                    case TokenType::Plus:
                        v0 = v1 + v2;
                        break;
                    case TokenType::Minus:
                        v0 = v1 - v2;
                        break;
                    case TokenType::Star:
                        v0 = v1 * v2;
                        break;
                    case TokenType::Divide:
                        if (v2 == 0) {
                            throw std::runtime_error("not a constant expression");
                        }
                        v0 = (int32_t)((int64_t)v1 / v2);
                        break;
                    default:
                        throw std::runtime_error("unsupported binary operator");
                    }
                    values.push_back(v0);
                }
                void visit_unary_expr(ASTN_UnaryExpr const& v) override {
                    // TODO...
                    throw std::runtime_error("not a constant expression");
                }
                void visit_call_expr(ASTN_CallExpr const& v) override {
                    throw std::runtime_error("not a constant expression");
                }
                void visit_literal_expr(ASTN_LiteralExpr const& v) override {
                    if (v.value.type != TokenType::IntLiteral) {
                        throw std::runtime_error("not a constant expression");
                    }
                    values.push_back(std::atoll(v.value.str.c_str()));
                }

                std::vector<int32_t> values;
            };
            ConstExprVisitor visitor;
            expr.accept(visitor);
            return visitor.values.back();
        }

        size_t append_byte(uint8_t v) {
            auto write_start = size(m_bytes);
            m_bytes.push_back(v);
            return write_start;
        }
        size_t append_dword(int32_t v) {
            auto write_start = size(m_bytes);
            for (int i = 0; i < 4; i++) {
                m_bytes.push_back(static_cast<uint8_t>(v));
                v >>= 8;
            }
            return write_start;
        }
        void write_dword(size_t pos, int32_t v) {
            for (int i = 0; i < 4; i++) {
                m_bytes[pos] = static_cast<uint8_t>(v);
                v >>= 8;
                pos++;
            }
        }

        void macro_add_imm(int32_t imm) {
            append_byte(ByteCodeType::PushDword);
            append_dword(imm);
            append_byte(ByteCodeType::Add);
        }
        void macro_adjust_sp(int32_t imm) {
            append_byte(ByteCodeType::AdjustStackRefConst);
            append_dword(imm);
        }

        void visit_decl_list(ASTN_DeclList const& v) override {
            for (auto const& decl : v.decls) {
                decl->accept(*this);
            }
        }
        void visit_var_decl(ASTN_VarDecl const& v) override {
            auto& cur_frame = m_frames.back();
            if (frame_find_id(cur_frame, v.id.str)) {
                throw std::runtime_error("redefinition of identifier");
            }
            if (std::get_if<ASTData_Type_Void>(&v.type.t)) {
                throw std::runtime_error("invalid variable type");
            }
            if (auto t = std::get_if<ASTData_Type_Array>(&v.type.t)) {
                if (!std::get_if<ASTData_Type_Int>(&t->inner->t)) {
                    throw std::runtime_error("array element must be of type int");
                }
                cur_frame.cur_sp += 4 * evaluate_constant_expr(*t->dimension);
                cur_frame.ids.push_back({ v.id.str, &v.type, cur_frame.cur_sp });
            }
            else {
                // Assume int
                cur_frame.cur_sp += 4;
                cur_frame.ids.push_back({ v.id.str, &v.type, cur_frame.cur_sp });
            }
        }
        void visit_func_decl(ASTN_FuncDecl const& v) override {
            if (std::get_if<ASTData_Type_Array>(&v.ret_type.t)) {
                throw std::runtime_error("invalid function return type");
            }
            if (auto entry = global_find_func(v.id.str)) {
                if (*entry->ret_type != v.ret_type || *entry->params != v.params) {
                    throw std::runtime_error("function signature mismatch");
                }
                if (v.body && entry->code_offset != -1) {
                    throw std::runtime_error("function body defined more than once");
                }
                // Do nothing
                return;
            }
            int code_offset{ -1 };
            if (v.body) {
                code_offset = (int)size(m_bytes);
                m_next_is_func_body = true;
            }
            m_funcs.push_back({ v.id.str, &v.ret_type, &v.params, code_offset });
            auto& cur_func = m_funcs.back();
            if (v.body) {
                // For nested functions, add jumps and fix pos
                append_byte(ByteCodeType::Jump);
                auto fixup_pos = append_dword(PENDING_FIXUP);
                cur_func.code_offset = (int)size(m_bytes);
                v.body->accept(*this);
                write_dword(fixup_pos, get_cur_code_pos());

                cur_func.associated_frame = &m_frames.back();
            }
        }
        void visit_expr_stmt(ASTN_ExprStmt const& v) override {
            auto& cur_frame = m_frames.back();
            auto old_sp = cur_frame.cur_sp;
            v.expr->accept(*this);
            bool is_void{};
            while (cur_frame.cur_sp > old_sp) {
                append_byte(ByteCodeType::PopDword);
                cur_frame.cur_sp -= 4;
            }
        }
        void visit_while_stmt(ASTN_WhileStmt const& v) override {
            auto& cur_frame = m_frames.back();
            uint32_t restart_pos = get_cur_code_pos();
            v.cond->accept(*this);
            if (m_expr_is_void) {
                throw std::runtime_error("expression shall not evaluate to void");
            }
            convert_id_expr_to_rvalue();
            append_byte(ByteCodeType::PushDword);
            append_dword(0);
            append_byte(ByteCodeType::CmpE);
            append_byte(ByteCodeType::JumpCond);
            cur_frame.cur_sp -= 4;
            auto fixup_pos = append_dword(PENDING_FIXUP);
            v.body->accept(*this);
            append_byte(ByteCodeType::Jump);
            append_dword(restart_pos);
            write_dword(fixup_pos, get_cur_code_pos());
        }
        void visit_if_stmt(ASTN_IfStmt const& v) override {
            auto& cur_frame = m_frames.back();
            v.cond->accept(*this);
            if (m_expr_is_void) {
                throw std::runtime_error("expression shall not evaluate to void");
            }
            convert_id_expr_to_rvalue();
            if (v.else_body) {
                append_byte(ByteCodeType::JumpCond);
                cur_frame.cur_sp -= 4;
                auto body_fixup_pos = append_dword(PENDING_FIXUP);
                v.else_body->accept(*this);
                append_byte(ByteCodeType::Jump);
                auto end_fixup_pos = append_dword(PENDING_FIXUP);
                write_dword(body_fixup_pos, get_cur_code_pos());
                v.body->accept(*this);
                write_dword(end_fixup_pos, get_cur_code_pos());
            }
            else {
                append_byte(ByteCodeType::PushDword);
                append_dword(0);
                append_byte(ByteCodeType::CmpE);
                append_byte(ByteCodeType::JumpCond);
                cur_frame.cur_sp -= 4;
                auto end_fixup_pos = append_dword(PENDING_FIXUP);
                v.body->accept(*this);
                write_dword(end_fixup_pos, get_cur_code_pos());
            }
        }
        void visit_return_stmt(ASTN_ReturnStmt const& v) override {
            auto& cur_frame = m_frames.back();
            auto old_sp = cur_frame.cur_sp;
            FuncEntry* func_ctx = nullptr;
            uint32_t total_sp{};
            for (auto const& frame : m_frames | std::views::reverse) {
                total_sp += frame.cur_sp;
                if (frame.func_ctx) {
                    func_ctx = frame.func_ctx;
                    break;
                }
            }
            if (!func_ctx) {
                throw std::runtime_error("return statement shall not appear here");
            }
            bool returns_void = std::get_if<ASTData_Type_Void>(&func_ctx->ret_type->t);
            if (returns_void) {
                if (v.expr) {
                    v.expr->accept(*this);
                    if (!m_expr_is_void) {
                        throw std::runtime_error("cannot return non-void value in void function");
                    }
                }

                while (total_sp > 0) {
                    append_byte(ByteCodeType::PopDword);
                    total_sp -= 4;
                }
                append_byte(ByteCodeType::Ret);
            }
            else {
                if (!v.expr) {
                    throw std::runtime_error("must return an value in non-void function");
                }

                v.expr->accept(*this);
                if (m_expr_is_void) {
                    throw std::runtime_error("cannot return void value in non-void function");
                }
                total_sp += 4;
                convert_id_expr_to_rvalue();

                while (total_sp < 12) {
                    append_byte(ByteCodeType::DuplicateDword);
                    total_sp += 4;
                }
                macro_adjust_sp(total_sp);
                append_byte(ByteCodeType::PushStackRef);
                macro_add_imm(-(int32_t)total_sp);
                append_byte(ByteCodeType::ReadRefDword);
                append_byte(ByteCodeType::RetDword);
            }

            cur_frame.cur_sp = old_sp;
        }
        void visit_compound_stmt(ASTN_CompoundStmt const& v) override {
            bool m_is_func_body = std::exchange(m_next_is_func_body, false);
            // NOTE: Compound stmts have one hidden arg: previous stack pointer
            m_frames.push_back(BlockFrame{ .parent = &m_frames.back() });
            auto& cur_frame = m_frames.back();

            // Push previous stack pointer into stack
            if (m_is_func_body) {
                // Function block
                cur_frame.func_ctx = &m_funcs.back();
                int args_size = 4 * cur_frame.func_ctx->params->size();
                append_byte(ByteCodeType::PushStackRef);
                macro_add_imm(4);
                append_byte(ByteCodeType::ReadRefDword);
                cur_frame.cur_sp += 4;
                // Add arguments into table
                for (int i = 0; i < (int)cur_frame.func_ctx->params->size(); i++) {
                    auto const& param = (*cur_frame.func_ctx->params)[i];
                    cur_frame.ids.push_back({ param.param.str, &param.type, -4 * (i + 2), param.is_arr });
                }
            }
            else {
                // Normal block
                append_byte(ByteCodeType::PushStackRef);
                macro_add_imm(cur_frame.parent->cur_sp);
                cur_frame.cur_sp += 4;
            }

            auto last_sp = cur_frame.cur_sp;
            for (auto const& decl : v.decls) {
                decl->accept(*this);
            }
            while (last_sp < cur_frame.cur_sp) {
                append_byte(ByteCodeType::PushDword);
                append_dword(0);
                last_sp += 4;
            }
            for (auto const& stmt : v.stmts) {
                stmt->accept(*this);
            }

            while (cur_frame.cur_sp > 0) {
                append_byte(ByteCodeType::PopDword);
                cur_frame.cur_sp -= 4;
            }

            if (m_is_func_body) {
                // Generate (fallback) return instruction
                if (std::get_if<ASTData_Type_Void>(&cur_frame.func_ctx->ret_type->t)) {
                    append_byte(ByteCodeType::Ret);
                }
                else {
                    // Insert after return address
                    append_byte(ByteCodeType::DuplicateDword);
                    append_byte(ByteCodeType::PushStackRef);
                    append_byte(ByteCodeType::PushDword);
                    append_dword(4);
                    append_byte(ByteCodeType::Add);
                    append_byte(ByteCodeType::PushDword);
                    append_dword(0);        // Default return value
                    append_byte(ByteCodeType::WriteRefDword);
                    append_byte(ByteCodeType::Ret);
                }
            }

            m_frames.pop_back();
        }
        void visit_id_expr(ASTN_IdExpr const& v) override {
            auto& cur_frame = m_frames.back();
            auto* cur_frame_ptr = &cur_frame;

            if (auto func_entry = global_find_func(v.id.str)) {
                if (!v.arridxs.empty()) {
                    throw std::runtime_error("function cannot be used for array access");
                }

                // Return function address
                if (func_entry->code_offset == -1) {
                    throw std::runtime_error("function is used before definition");
                }
                append_byte(ByteCodeType::PushStackRef);
                macro_add_imm(cur_frame.cur_sp);
                if (func_entry->associated_frame) {
                    for (auto fp = &cur_frame; fp != func_entry->associated_frame; fp = fp->parent) {
                        macro_add_imm(-4);
                        append_byte(ByteCodeType::ReadRefDword);
                    }
                }
                append_byte(ByteCodeType::PushDword);
                append_dword(func_entry->code_offset + m_start_offset);

                cur_frame.cur_sp += 8;

                m_expr_is_void = false;
                m_expr_is_id = false;
                m_expr_is_void_fun = std::get_if<ASTData_Type_Void>(&func_entry->ret_type->t);

                return;
            }

            bool is_array{};
            bool is_param_array{};
            bool is_array_access{ !v.arridxs.empty() };

            if (is_array_access) {
                if (v.arridxs.size() > 1) {
                    throw std::runtime_error("multi-dimensional arrays are not supported");
                }
            }

            IdEntry* id_entry{};
            int layers_cnt{};
            for (auto& frame : m_frames | std::views::reverse) {
                id_entry = frame_find_id(frame, v.id.str);
                if (id_entry) { break; }
                layers_cnt++;
            }
            if (!id_entry) {
                m_logger->error(slogsrc(v.id, L"identifier `{}` not declared", winrt::to_hstring(v.id.str)));
                throw std::runtime_error("identifier not found");
            }
            is_param_array = id_entry->is_param_arr;
            if (std::get_if<ASTData_Type_Array>(&id_entry->type->t)) {
                is_array = true;
            }

            if ((!is_array && !is_param_array) && is_array_access) {
                throw std::runtime_error("variable is not an array");
            }

            // Resolve to variable address
            append_byte(ByteCodeType::PushStackRef);
            macro_add_imm(cur_frame.cur_sp);
            for (int i = 0; i < layers_cnt; i++) {
                cur_frame_ptr = cur_frame_ptr->parent;
                macro_add_imm(-4);
                append_byte(ByteCodeType::ReadRefDword);
            }
            macro_add_imm(-id_entry->offset);
            cur_frame.cur_sp += 4;

            if (is_array_access) {
                if (is_param_array) {
                    // HACK: Deref pointer to array
                    m_expr_is_id = true;
                    convert_id_expr_to_rvalue();
                }

                v.arridxs[0]->accept(*this);
                if (m_expr_is_void) {
                    throw std::runtime_error("void cannot be used as array index");
                }
                convert_id_expr_to_rvalue();
                append_byte(ByteCodeType::PushDword);
                append_dword(4);
                append_byte(ByteCodeType::Mul);
                append_byte(ByteCodeType::Add);
                cur_frame.cur_sp -= 4;
            }

            m_expr_is_void = false;
            m_expr_is_id = true;
            if (is_array && !is_array_access) {
                // HACK: Array as pointer
                m_expr_is_id = false;
            }
        }
        void convert_id_expr_to_rvalue() {
            if (m_expr_is_id) {
                m_expr_is_id = false;
                append_byte(ByteCodeType::ReadRefDword);
            }
        }
        void visit_binary_expr(ASTN_BinaryExpr const& v) override {
            auto& cur_frame = m_frames.back();
            bool coerce_id_expr{};
            bool is_assign{};

            if (v.op.type == TokenType::Assign) {
                is_assign = true;
            }
            else {
                coerce_id_expr = true;
            }

            v.left->accept(*this);
            if (m_expr_is_void) {
                throw std::runtime_error("void cannot participate in arithmetic");
            }
            if (is_assign && !m_expr_is_id) {
                throw std::runtime_error("cannot write to non l-value");
            }
            if (coerce_id_expr) {
                convert_id_expr_to_rvalue();
            }
            v.right->accept(*this);
            if (m_expr_is_void) {
                throw std::runtime_error("void cannot participate in arithmetic");
            }
            convert_id_expr_to_rvalue();
            switch (v.op.type) {
            case TokenType::Assign:
                append_byte(ByteCodeType::DuplicateDword);
                append_byte(ByteCodeType::PopDword);
                append_byte(ByteCodeType::WriteRefDword);
                append_byte(ByteCodeType::PushStackRef);
                macro_add_imm(-12);
                append_byte(ByteCodeType::ReadRefDword);
                break;
            case TokenType::LessEqual:
                append_byte(ByteCodeType::CmpLe);
                break;
            case TokenType::GreaterEqual:
                append_byte(ByteCodeType::CmpGe);
                break;
            case TokenType::LChevron:
                append_byte(ByteCodeType::CmpL);
                break;
            case TokenType::RChevron:
                append_byte(ByteCodeType::CmpG);
                break;
            case TokenType::Equal:
                append_byte(ByteCodeType::CmpE);
                break;
            case TokenType::NotEqual:
                append_byte(ByteCodeType::CmpNe);
                break;
            case TokenType::Plus:
                append_byte(ByteCodeType::Add);
                break;
            case TokenType::Minus:
                append_byte(ByteCodeType::Sub);
                break;
            case TokenType::Star:
                append_byte(ByteCodeType::Mul);
                break;
            case TokenType::Divide:
                append_byte(ByteCodeType::Div);
                break;
            default:
                throw std::runtime_error("unsupported binary operator");
            }

            cur_frame.cur_sp -= 4;

            m_expr_is_void = false;
            m_expr_is_id = false;
        }
        void visit_unary_expr(ASTN_UnaryExpr const& v) override {
            throw std::runtime_error("unsupported: not implemented");

            m_expr_is_void = false;
            m_expr_is_id = false;
        }
        void visit_call_expr(ASTN_CallExpr const& v) {
            auto& cur_frame = m_frames.back();
            auto old_sp = cur_frame.cur_sp;

            for (auto const& arg : v.args | std::views::reverse) {
                arg->accept(*this);
                convert_id_expr_to_rvalue();
            }
            m_expr_is_void_fun = {};
            v.callee->accept(*this);
            convert_id_expr_to_rvalue();

            append_byte(ByteCodeType::CallIndirect);

            bool is_void{ m_expr_is_void_fun };

            if (is_void) {
                cur_frame.cur_sp -= 4;
                while (cur_frame.cur_sp > old_sp) {
                    append_byte(ByteCodeType::PopDword);
                    cur_frame.cur_sp -= 4;
                }
            }
            else {
                auto stack_delta = cur_frame.cur_sp - old_sp;
                while (stack_delta < 12) {
                    append_byte(ByteCodeType::DuplicateDword);
                    stack_delta += 4;
                }
                // Move return value
                macro_adjust_sp(stack_delta);
                append_byte(ByteCodeType::PushStackRef);
                macro_add_imm(-stack_delta);
                append_byte(ByteCodeType::ReadRefDword);
                cur_frame.cur_sp = old_sp + 4;
            }

            m_expr_is_void = is_void;
            m_expr_is_id = false;
        }
        void visit_literal_expr(ASTN_LiteralExpr const& v) override {
            auto& cur_frame = m_frames.back();
            if (v.value.type != TokenType::IntLiteral) {
                throw std::runtime_error("unsupported literal type");
            }
            append_byte(ByteCodeType::PushDword);
            append_dword(std::atoll(v.value.str.c_str()));
            cur_frame.cur_sp += 4;

            m_expr_is_void = false;
            m_expr_is_id = false;
        }

        Logger* m_logger;
        int m_start_offset;
        std::list<BlockFrame> m_frames;
        //BlockFrame* m_cur_frame;
        std::list<FuncEntry> m_funcs;
        bool m_next_is_func_body{};
        bool m_expr_is_id{};
        bool m_expr_is_void{};
        bool m_expr_is_void_fun{};
        // Used to fix global variable references
        std::vector<int> m_global_fixups;
    };

    std::pair<std::vector<uint8_t>, CodeMetadata> CodeGenerator::ast_to_code(ASTN const& root_node, int start_offset) try {
        CodeGenAstVisitor visitor(m_logger, start_offset);
        visitor.start(root_node);
        return { std::move(visitor.m_bytes), std::move(visitor.m_code_meta) };
    }
    catch (std::runtime_error const& e) {
        m_logger->error(std::format(L"internal compiler error: {}", winrt::to_hstring(e.what())));
        throw;
    }
}
