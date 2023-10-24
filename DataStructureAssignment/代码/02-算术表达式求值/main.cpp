#include <iostream>
#include <optional>
#include <fstream>
#include <format>
#include <string>
#include <vector>
#include <stack>

#define fmt_print(...) (::std::cout << ::std::format(__VA_ARGS__))
#define fmt_println(...) (::std::cout << ::std::format(__VA_ARGS__) << ::std::endl)
#define fmt_write(fs, ...) ((fs) << ::std::format(__VA_ARGS__))
#define fmt_writeln(fs, ...) ((fs) << ::std::format(__VA_ARGS__) << ::std::endl)

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

bool input_yesno(std::string_view prompt, bool default_yes) {
    std::string input;
    bool b = default_yes;
    while (true) {
        std::cout << prompt << (default_yes ? " [Y/n] " : " [y/N] ");
        if (!getline(std::cin, input)) { break; }
        if (input == "") {
            // No input, use default value
            break;
        }
        else if (input == "y" || input == "Y") {
            b = true;
            break;
        }
        else if (input == "n" || input == "N") {
            b = false;
            break;
        }
        // Otherwise, input is invalid; retry input
    }
    return b;
}

struct parse_error : std::exception {
    parse_error(const std::string& msg, size_t in_loc) :
        std::exception(std::format("{} (at position {})", msg, in_loc + 1).c_str()), loc(in_loc) {}
    size_t loc;
};

void skip_whitespace(std::string_view sv, size_t &rpos) {
    while (rpos < sv.size() && std::isspace(sv[rpos])) {
        rpos++;
    }
}
char look_ahead(std::string_view sv, size_t &rpos) {
    if (rpos == sv.size()) {
        throw std::runtime_error("Unexpected end of stream while parsing");
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
// NOTE: Copied from JSON parser (Project-01)
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
                throw parse_error("解析数值时检测到状态机的损坏", rpos);
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
        throw parse_error("发现了无效的数值", rpos);
    }
    rpos = cur_rpos;
    return (neg_sign ? -1 : 1) * (int_num + frac_num) * npow(10, (neg_exp_pow ? -1 : 1) * exp_pow);
}
double evaluate_expression(std::string_view expr) {
    std::stack<std::pair<double, size_t>> opnd_stack;
    std::stack<std::pair<char, size_t>> optr_stack;
    size_t last_rpos = 0;
    size_t cur_rpos = 0;
    bool last_pushed_opnd, last_pushed_optr;
    auto print_input_expr_change_fn = [&]() {
        if (last_rpos != cur_rpos) {
            fmt_println("* 输入序列已变更为 `{}`。", expr.substr(cur_rpos));
            last_rpos = cur_rpos;
        }
    };
    auto push_opnd_fn = [&](double v, size_t loc) {
        fmt_println("* 正在将 `{}` 入栈(操作数)...", v);
        opnd_stack.emplace(v, loc);
        last_pushed_opnd = true;
        last_pushed_optr = false;
    };
    auto pop_opnd_fn = [&]() {
        auto result = opnd_stack.top();
        fmt_println("* 正在将 `{}` 出栈(操作数)...", result.first);
        opnd_stack.pop();
        return result;
    };
    auto push_optr_fn = [&](char v, size_t loc) {
        fmt_println("* 正在将 `{}` 入栈(运算符)...", v);
        optr_stack.emplace(v, loc);
        last_pushed_opnd = false;
        last_pushed_optr = true;
    };
    auto pop_optr_fn = [&]() {
        auto result = optr_stack.top();
        fmt_println("* 正在将 `{}` 出栈(运算符)...", result.first);
        optr_stack.pop();
        return result;
    };
    auto get_ops_priority_fn = [](char op1, char op2) -> std::optional<int> {
        constexpr int INVALID = -2, LT = -1, GT = 1, EQ = 0;
        auto op_to_map_idx_fn = [](char op) {
            switch (op) {
            case '+':       return 0;
            case '-':       return 1;
            case '*':       return 2;
            case '/':       return 3;
            case '(':       return 4;
            case ')':       return 5;
            case '#':       return 6;
            }
            return -1;
        };
        const int rels_map[7][7] = {
            GT, GT, LT, LT, LT, GT, GT,
            GT, GT, LT, LT, LT, GT, GT,
            GT, GT, GT, GT, LT, GT, GT,
            GT, GT, GT, GT, LT, GT, GT,
            LT, LT, LT, LT, LT, EQ, INVALID,
            GT, GT, GT, GT, INVALID, GT, GT,
            LT, LT, LT, LT, LT, INVALID, EQ,
        };
        int idx1 = op_to_map_idx_fn(op1), idx2 = op_to_map_idx_fn(op2);
        if (idx1 < 0 || idx2 < 0) { return std::nullopt; }
        int result = rels_map[idx1][idx2];
        if (result == INVALID) { return std::nullopt; }
        return result;
    };
    auto ordinary_binary_pop_calc = [&](char ch) {
        // NOTE: Returns whether a binary operation is handled
        std::pair<double, size_t> opnd1, opnd2;
        switch (ch) {
        case '+':
            opnd2 = pop_opnd_fn(); opnd1 = pop_opnd_fn();
            push_opnd_fn(opnd1.first + opnd2.first, opnd1.second);
            break;
        case '-':
            opnd2 = pop_opnd_fn(); opnd1 = pop_opnd_fn();
            push_opnd_fn(opnd1.first - opnd2.first, opnd1.second);
            break;
        case '*':
            opnd2 = pop_opnd_fn(); opnd1 = pop_opnd_fn();
            push_opnd_fn(opnd1.first * opnd2.first, opnd1.second);
            break;
        case '/':
            opnd2 = pop_opnd_fn(); opnd1 = pop_opnd_fn();
            push_opnd_fn(opnd1.first / opnd2.first, opnd1.second);
            break;
        default:
            return false;
        }
        return true;
    };
    skip_whitespace(expr, cur_rpos);
    if (next_char(expr, cur_rpos) != '#') {
        throw parse_error("表达式未以 `#` 开头", cur_rpos);
    }
    push_optr_fn('#', cur_rpos - 1);
    print_input_expr_change_fn();
    while (true) {
        skip_whitespace(expr, cur_rpos);
        char ch = look_ahead(expr, cur_rpos);
        const size_t cur_loc = cur_rpos;
        if (ch == '#') {
            // Found terminator; process remaining elements in stack
            if (last_pushed_optr) {
                throw parse_error("预期得到表达式，实际得到 `#`", cur_loc);
            }
            cur_rpos++;
            std::pair<char, size_t> optr;
            while ((optr = pop_optr_fn()).first != '#') {
                if (optr.first == '(') {
                    throw parse_error("`(` 没有对应的闭合符", optr.second);
                }
                if (!ordinary_binary_pop_calc(optr.first)) {
                    throw parse_error(std::format("预料之外的符号 `{}`", optr.first), optr.second);
                }
            }
            break;
        }
        else if (std::isdigit(ch)) {
            if (last_pushed_opnd) {
                throw parse_error("预期得到运算符，实际得到数值", cur_loc);
            }
            push_opnd_fn(read_number(expr, cur_rpos), cur_loc);
        }
        else if (ch == '(') {
            if (last_pushed_opnd) {
                throw parse_error("预期得到运算符，实际得到 `(`", cur_loc);
            }
            cur_rpos++;
            push_optr_fn('(', cur_loc);
        }
        else if (ch == ')') {
            // Process current elements until `(` is poped
            if (last_pushed_optr) {
                throw parse_error("预期得到表达式，实际得到 `)`", cur_loc);
            }
            cur_rpos++;
            std::pair<double, size_t> opnd1, opnd2;
            std::pair<char, size_t> optr;
            while ((optr = pop_optr_fn()).first != '(') {
                if (optr.first == '#') {
                    throw parse_error("`)` 没有对应的闭合符", cur_loc);
                }
                if (!ordinary_binary_pop_calc(optr.first)) {
                    throw parse_error(std::format("预料之外的符号 `{}`", optr.first), optr.second);
                }
            }
        }
        else {
            // Try ordinary calculations
            if (last_pushed_optr) {
                throw parse_error(std::format("预期得到表达式，实际得到 `{}`", ch), cur_loc);
            }
            cur_rpos++;
            while (true) {
                auto orel = get_ops_priority_fn(optr_stack.top().first, ch);
                if (!orel) {
                    throw parse_error(std::format("运算符 `{}` 无效", ch), cur_loc);
                }
                auto rel = *orel;
                if (rel < 0) { break; }
                auto optr = pop_optr_fn();
                if (!ordinary_binary_pop_calc(optr.first)) {
                    throw parse_error(std::format("预料之外的符号 `{}`", ch), cur_loc);
                }
            }
            push_optr_fn(ch, cur_loc);
        }
        print_input_expr_change_fn();
    }
    print_input_expr_change_fn();
    skip_whitespace(expr, cur_rpos);
    if (cur_rpos != expr.size()) {
        throw parse_error("找到了预料之外的非空白字符", cur_rpos);
    }
    return pop_opnd_fn().first;
}

void main_loop(void) {
    while (true) {
        std::string expression;
        if (input_yesno("要从文件读取吗?", false)) {
            std::string path;
            fmt_print("输入文件路径(不要带双引号): ");
            if (!getline(std::cin, path)) {
                fmt_println("错误: 输入无效。请重试。");
                continue;
            }
            std::ifstream fs(path);
            if (!fs) {
                fmt_println("错误: 无法打开文件");
                continue;
            }
            while (true) {
                char ch;
                if (!fs.get(ch)) { break; }
                expression += ch;
            }
        }
        else {
            fmt_print("输入表达式: ");
            if (!getline(std::cin, expression)) {
                fmt_println("错误: 输入无效。请重试。");
                continue;
            }
        }
        try {
            fmt_println("{} 的结果是: {}", expression, evaluate_expression(expression));
        }
        catch (const std::exception& e) {
            fmt_println("计算表达式 `{}` 时发生了错误: {}", expression, e.what());
        }
    }
}

int main(void) try {
    main_loop();
    // system("pause");
}
catch (const std::exception& e) {
    fmt_println("!!! 发生了无法处理的严重错误(std::exception: {})，程序将会退出。", e.what());
    // system("pause");
    return EXIT_FAILURE;
}
catch (...) {
    fmt_println("!!! 发生了无法处理的严重错误，程序将会退出。");
    // system("pause");
    return EXIT_FAILURE;
}
