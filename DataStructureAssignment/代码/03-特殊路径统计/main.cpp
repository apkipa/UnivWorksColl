#include <iostream>
#include <format>
#include <vector>
#include <stack>

#define fmt_print(...) (::std::cout << ::std::format(__VA_ARGS__))
#define fmt_println(...) (::std::cout << ::std::format(__VA_ARGS__) << ::std::endl)
#define fmt_write(fs, ...) ((fs) << ::std::format(__VA_ARGS__))
#define fmt_writeln(fs, ...) ((fs) << ::std::format(__VA_ARGS__) << ::std::endl)

size_t n;
std::vector<size_t> parents;

size_t find_lca(size_t na, size_t nb) {
    if (na == nb) { return na; }
    std::stack<size_t> patha, pathb;
    size_t last_pop_na, last_pop_nb;
    while (na != 0) {
        patha.push(na);
        na = parents[na];
    }
    while (nb != 0) {
        pathb.push(nb);
        nb = parents[nb];
    }
    while (!patha.empty() && !pathb.empty()) {
        size_t cur_pop_na = patha.top(); patha.pop();
        size_t cur_pop_nb = pathb.top(); pathb.pop();
        if (cur_pop_na != cur_pop_nb) { break; }
        last_pop_na = cur_pop_na;
        last_pop_nb = cur_pop_nb;
    }
    return last_pop_na;
}

bool check(size_t na, size_t nb) {
    if (na >= nb) { return false; }
    const size_t low = na, high = nb;
    const size_t lca = find_lca(na, nb);
    auto in_range_fn = [&](size_t v) {
        return low <= v && v <= high;
    };
    while (na != lca) {
        na = parents[na];
        if (!in_range_fn(na)) { return false; }
    }
    while (nb != lca) {
        nb = parents[nb];
        if (!in_range_fn(nb)) { return false; }
    }
    return true;
}

int main(void) try {
    std::cin >> n;
    parents.resize(n + 1);
    for (size_t i = 0; i < n; i++) {
        std::cin >> parents[i + 1];
    }
    size_t count = 0;
    for (size_t i = 0; i < n; i++) {
        for (size_t j = i + 1; j < n; j++) {
            count += check(i + 1, j + 1);
        }
    }
    fmt_println("{}", count);
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
