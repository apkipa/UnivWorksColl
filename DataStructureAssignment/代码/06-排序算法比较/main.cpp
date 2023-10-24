#include <iostream>
#include <fstream>
#include <numeric>
#include <chrono>
#include <format>
#include <random>
#include <ranges>
#include <queue>
#include <stack>
#include <span>

#define fmt_print(...) (::std::cout << ::std::format(__VA_ARGS__))
#define fmt_println(...) (::std::cout << ::std::format(__VA_ARGS__) << ::std::endl)
#define fmt_write(fs, ...) ((fs) << ::std::format(__VA_ARGS__))
#define fmt_writeln(fs, ...) ((fs) << ::std::format(__VA_ARGS__) << ::std::endl)

int gen_random_number(int start, int end) {
    static std::mt19937 gen{ std::random_device{}() };
    return std::uniform_int_distribution<int>{ start, end }(gen);
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

template<typename Functor>
double eval_elapsed_time(Functor&& functor) {
    auto ts = std::chrono::high_resolution_clock::now();
    functor();
    auto dur = std::chrono::high_resolution_clock::now() - ts;
    return std::chrono::duration<double>(dur).count();
}

void insertion_sort(std::vector<int>& vec) {
    using std::swap;
    const size_t count = vec.size();
    for (size_t i = 1; i < count; i++) {
        auto elem = std::move(vec[i]);
        size_t pos = i;
        while (pos > 0 && elem < vec[pos - 1]) {
            vec[pos] = std::move(vec[pos - 1]);
            pos--;
        }
        vec[pos] = std::move(elem);
    }
}
void shell_sort(std::vector<int>& vec) {
    using std::swap;
    const size_t count = vec.size();
    const size_t steps[] = { 109, 41, 19, 5, 1 };
    auto sort_with_step_fn = [&](size_t step) {
        for (size_t i = step; i < count; i++) {
            auto elem = std::move(vec[i]);
            size_t pos = i;
            while (pos >= step && elem < vec[pos - step]) {
                vec[pos] = std::move(vec[pos - step]);
                pos -= step;
            }
            vec[pos] = std::move(elem);
        }
    };
    for (const auto& step : steps) {
        sort_with_step_fn(step);
    }
}
void bubble_sort(std::vector<int>& vec) {
    using std::swap;
    const size_t count = vec.size();
    for (size_t i = 0; i < count; i++) {
        for (size_t j = 1; j < count - i; j++) {
            if (vec[j - 1] > vec[j]) {
                swap(vec[j - 1], vec[j]);
            }
        }
    }
}
void quick_sort(std::vector<int>& vec) {
    using std::swap;
    const size_t count = vec.size();
    std::stack<std::pair<size_t, size_t>> parts;
    auto partition_fn = [&](size_t start, size_t end) -> size_t {
        auto pivot = std::move(vec[start]);
        end--;
        while (start < end) {
            while (start < end && vec[end] >= pivot) {
                end--;
            }
            vec[start] = std::move(vec[end]);
            while (start < end && vec[start] <= pivot) {
                start++;
            }
            vec[end] = std::move(vec[start]);
        }
        vec[end] = std::move(pivot);
        return end;
    };
    parts.emplace(0, count);
    while (!parts.empty()) {
        auto [start, end] = parts.top(); parts.pop();
        if (end - start <= 1) { continue; }
        auto pivot_idx = partition_fn(start, end);
        parts.emplace(start, pivot_idx);
        parts.emplace(pivot_idx + 1, end);
    }
}
void selection_sort(std::vector<int>& vec) {
    using std::swap;
    const size_t count = vec.size();
    for (size_t i = 0; i < count; i++) {
        size_t min_idx = i;
        for (size_t j = i + 1; j < count; j++) {
            if (vec[j] < vec[min_idx]) {
                min_idx = j;
            }
        }
        swap(vec[i], vec[min_idx]);
    }
}
void heap_sort(std::vector<int>& vec) {
    using std::swap;
    const size_t count = vec.size();
    auto sift_down_fn = [&](size_t start, size_t end) {
        size_t child = start * 2 + 1;
        while (child < end) {
            if (child + 1 < end && vec[child + 1] > vec[child]) {
                child++;
            }
            if (vec[start] >= vec[child]) { break; }
            swap(vec[start], vec[child]);
            start = child;
            child = start * 2 + 1;
        }
    };
    for (size_t i = (count - 1 - 1) / 2; i > 0; i--) {
        sift_down_fn(i, count);
    }
    sift_down_fn(0, count);
    for (size_t i = count - 1; i > 0; i--) {
        swap(vec[0], vec[i]);
        sift_down_fn(0, i);
    }
}
void merge_sort(std::vector<int>& vec) {
    using std::swap;
    const size_t count = vec.size();
    std::stack<std::pair<size_t, size_t>> parts;
    std::vector<int> workbuf(count);
    parts.emplace(0, count);
    while (!parts.empty()) {
        auto [start, end] = parts.top(); parts.pop();
        // NOTE: For every interval, the split plan is always identical
        if (start > end) {
            // Stage 2: merge
            swap(start, end);
            const size_t midpos = start + (end - start) / 2;
            size_t ia = start, ib = midpos, buf_idx = 0;
            while (ia < midpos && ib < end) {
                if (vec[ia] < vec[ib]) {
                    workbuf[buf_idx++] = std::move(vec[ia++]);
                }
                else {
                    workbuf[buf_idx++] = std::move(vec[ib++]);
                }
            }
            while (ia < midpos) {
                workbuf[buf_idx++] = std::move(vec[ia++]);
            }
            while (ib < end) {
                workbuf[buf_idx++] = std::move(vec[ib++]);
            }
            buf_idx = 0;
            for (size_t i = start; i < end; i++) {
                vec[i] = std::move(workbuf[buf_idx++]);
            }
        }
        else {
            // Stage 1: split
            if (end - start <= 1) {
                continue;
            }
            const size_t midpos = start + (end - start) / 2;
            // Store reversed pair to indicate the interval should
            // be merged in future iterations
            parts.emplace(end, start);
            parts.emplace(start, midpos);
            parts.emplace(midpos, end);
        }
    }
}
void radix_sort(std::vector<int>& vec) {
    // Assuming all values are positive
    if (vec.size() <= 1) { return; }
    struct Node {
        int value;
        Node* next;
    };
    Node *nhead, *ncur, *ntail;
    // Vector -> LinkedList
    nhead = ntail = new Node{ vec.front(), nullptr };
    for (auto i : vec | std::views::drop(1)) {
        ntail->next = new Node{ i, nullptr };
        ntail = ntail->next;
    }
    // Sort LinkedList
    {
        // value(head, tail)
        std::pair<Node*, Node*> buckets[10];
        constexpr int loop_count = [] {
            int temp = std::numeric_limits<int>::max();
            int n = 1;
            while (temp >= 10) {
                temp /= 10;
                n++;
            }
            return n;
        }();
        int temp = 1;
        for (int i = 0; i < loop_count; i++, temp *= 10) {
            for (auto& bucket : buckets) { bucket = {}; }
            auto bucket_append_fn = [&](int index) {
                Node *ncur = nhead;
                nhead = nhead->next;
                ncur->next = nullptr;
                auto& [nbhead, nbtail] = buckets[index];
                if (!nbhead) {
                    nbhead = nbtail = ncur;
                }
                else {
                    nbtail->next = ncur;
                    nbtail = ncur;
                }
            };
            while (nhead) {
                bucket_append_fn((nhead->value / temp) % 10);
            }
            // Collect buckets
            nhead = nullptr;
            for (auto& bucket : buckets) {
                auto& [nbhead, nbtail] = bucket;
                if (!nbhead) { continue; }
                if (!nhead) {
                    std::tie(nhead, ntail) = { nbhead, nbtail };
                }
                else {
                    ntail->next = nbhead;
                    ntail = nbtail;
                }
            }
        }
    }
    // LinkedList -> Vector
    ncur = nhead;
    auto it = vec.begin();
    while (ncur) {
        *it++ = ncur->value;
        ncur = ncur->next;
    }
    // Free LinkedList
    ncur = nhead;
    while (ncur) {
        nhead = ncur;
        ncur = ncur->next;
        delete nhead;
    }
}

void test_sort_functions(const char* sample_path, size_t iterations) {
    fmt_println("----- 测试样本 {} -----", sample_path);
    std::vector<int> sample_sorted;
    std::vector<int> sample;
    {
        int temp;
        std::ifstream sample_fs(sample_path);
        if (!sample_fs) {
            fmt_println("错误: 无法读取 `{}`", sample_path);
            return;
        }
        while (sample_fs >> temp) {
            sample.push_back(temp);
        }
        sample_sorted = sample;
        std::sort(sample_sorted.begin(), sample_sorted.end());
    }
    auto test_sort_fun_verbose_fn = [&](auto&& fn, std::string_view fn_name) {
        std::vector<double> eval_durations;
        std::vector<int> tmpvec;
        // const size_t warmup_iteration = 1;
        const size_t warmup_iteration = 0;
        for (size_t i = 0; i < warmup_iteration; i++) {
            tmpvec = sample;
            fmt_print("        \r{}: 正在预热... ({} / {})", fn_name, i + 1, warmup_iteration);
            std::cout.flush();
            fn(tmpvec);
            if (tmpvec != sample_sorted) {
                throw std::runtime_error("排序算法存在错误");
            }
        }
        for (size_t i = 0; i < iterations; i++) {
            tmpvec = sample;
            fmt_print("        \r{}: 正在测评... ({} / {})", fn_name, i + 1, iterations);
            std::cout.flush();
            eval_durations.push_back(eval_elapsed_time([&]() { fn(tmpvec); }));
            if (tmpvec != sample_sorted) {
                throw std::runtime_error("排序算法存在错误");
            }
        }
        endl(std::cout);
        return eval_durations;
    };
    auto baseline_dur = test_sort_fun_verbose_fn(
        [](std::vector<int>& v) { std::sort(v.begin(), v.end()); }, "std::sort");
    auto insertion_dur = test_sort_fun_verbose_fn(insertion_sort, "insertion_sort");
    auto shell_dur = test_sort_fun_verbose_fn(shell_sort, "shell_sort");
    auto bubble_dur = test_sort_fun_verbose_fn(bubble_sort, "bubble_sort");
    auto quick_dur = test_sort_fun_verbose_fn(quick_sort, "quick_sort");
    auto selection_dur = test_sort_fun_verbose_fn(selection_sort, "selection_sort");
    auto heap_dur = test_sort_fun_verbose_fn(heap_sort, "heap_sort");
    auto merge_dur = test_sort_fun_verbose_fn(merge_sort, "merge_sort");
    auto radix_dur = test_sort_fun_verbose_fn(radix_sort, "radix_sort");
    fmt_println("* 统计信息:");
    auto print_stat_dur_fn = [&](const std::vector<double>& dur, std::string_view name) {
        double baseline_dur_sum = std::accumulate(baseline_dur.begin(), baseline_dur.end(), 0.0);
        double dur_sum = std::accumulate(dur.begin(), dur.end(), 0.0);
        double baseline_speed = baseline_dur.size() / baseline_dur_sum;
        double speed = dur.size() / dur_sum;
        double relative_speed = (speed / baseline_speed - 1) * 100;
        if (fabs(relative_speed) < 1e-6) {
            relative_speed = 0;
        }
        fmt_println(
            "* {}: {:.6f} s / {} op{} ({:+.2f}%)",
            name, dur_sum, dur.size(), dur.size() == 1 ? "" : "s",
            relative_speed
        );
    };
    print_stat_dur_fn(baseline_dur, "std::sort(基准)");
    print_stat_dur_fn(insertion_dur, "insertion_sort");
    print_stat_dur_fn(shell_dur, "shell_sort");
    print_stat_dur_fn(bubble_dur, "bubble_sort");
    print_stat_dur_fn(quick_dur, "quick_sort");
    print_stat_dur_fn(selection_dur, "selection_sort");
    print_stat_dur_fn(heap_dur, "heap_sort");
    print_stat_dur_fn(merge_dur, "merge_sort");
    print_stat_dur_fn(radix_dur, "radix_sort");
    fmt_println("--------------------");
}

int main(void) try {
    if (input_yesno("需要生成样本吗?", false)) {
        fmt_println("正在生成样本，请稍后...");
        for (size_t i = 0; i < 10; i++) {
            const size_t count = 50000;
            std::vector<int> vec;
            vec.reserve(count);
            for (size_t j = 0; j < count; j++) {
                vec.push_back(gen_random_number(0, 100000000));
            }
            switch (i) {
            case 0:
                std::sort(vec.begin(), vec.end());
                break;
            case 1:
                std::sort(vec.begin(), vec.end(), std::greater<int>{});
                break;
            default:
                break;
            }
            std::ofstream sample_fs(std::format("sample{:02}.txt", i + 1));
            sample_fs.exceptions(std::ios::badbit);
            bool first = true;
            for (const auto& j : vec) {
                if (!first) { sample_fs << ' '; }
                sample_fs << j;
                first = false;
            }
        }
    }
    fmt_println("正在准备测试...");
    for (size_t i = 0; i < 10; i++) {
        test_sort_functions(std::format("sample{:02}.txt", i + 1).c_str(), 1);
    }
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
