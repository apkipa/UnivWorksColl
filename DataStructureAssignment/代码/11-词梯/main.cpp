#include <unordered_set>
#include <unordered_map>
#include <string_view>
#include <algorithm>
#include <optional>
#include <iostream>
#include <fstream>
#include <format>
#include <vector>
#include <ranges>
#include <queue>
#include <stack>

#define fmt_print(...) (::std::cout << ::std::format(__VA_ARGS__))
#define fmt_println(...) (::std::cout << ::std::format(__VA_ARGS__) << ::std::endl)
#define fmt_write(fs, ...) ((fs) << ::std::format(__VA_ARGS__))
#define fmt_writeln(fs, ...) ((fs) << ::std::format(__VA_ARGS__) << ::std::endl)

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

void to_lower_str_inplace(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(),
        [](auto c){ return static_cast<char>(std::tolower(c)); });
}
std::string to_lower_str(std::string str) {
    to_lower_str_inplace(str);
    return str;
}

struct WordLadderGraph {
    void add_word(std::string_view word) {
        size_t idx = find_word(word);
        if (idx != m_vertices.size()) {
            // Word already exists
            return;
        }
        m_vertices.push_back({ std::string(word), {}, false });
        m_word_idx_map[m_vertices.back().word] = m_vertices.size() - 1;
        // Also add auxiliary nodes
        std::string tmpword{ word };
        for (auto& ch : tmpword) {
            auto old_ch = ch;
            ch = '*';
            size_t aux_idx = find_word(tmpword);
            if (aux_idx == m_vertices.size()) {
                m_vertices.push_back({ tmpword, {}, true });
                m_word_idx_map[m_vertices.back().word] = m_vertices.size() - 1;
            }
            // Add edges between actual words and auxiliary bridge words
            m_vertices[idx].adj_dsts.push_back(aux_idx);
            m_vertices[aux_idx].adj_dsts.push_back(idx);
            ch = old_ch;
        }
    }

    bool word_in_graph(std::string_view word) const {
        return find_word(word) != m_vertices.size();
    }

    // Returns a list of shortest routes
    std::vector<std::vector<std::string>> find_route(
        std::string_view word_start, std::string_view word_end) const
    {
        // Preparation
        std::vector<bool> visited(m_vertices.size());
        std::vector<std::vector<size_t>> path(m_vertices.size());
        std::queue<size_t> search_queue;
        size_t word_start_idx = find_word(word_start), word_end_idx = find_word(word_end);
        if (word_end_idx == m_vertices.size()) {
            // End word not in list, early exit
            return {};
        }
        // NOTE: Start word is allowed not to appear in graph
        if (word_start_idx != m_vertices.size()) {
            visited[word_start_idx] = true;
        }
        // Actual search (BFS)
        {
            std::string tmpword{ word_start };
            for (auto& ch : tmpword) {
                auto old_ch = ch;
                ch = '*';
                auto idx = find_word(tmpword);
                if (idx != m_vertices.size()) {
                    visited[idx] = true;
                    search_queue.push(idx);
                }
                ch = old_ch;
            }
        }
        while (!visited[word_end_idx] && !search_queue.empty()) {
            const auto iterate_count = search_queue.size();
            std::unordered_set<size_t> nodes_to_mark_visited;
            for (size_t i = 0; i < iterate_count; i++) {
                auto cur_idx = search_queue.front(); search_queue.pop();
                for (const auto& dest_idx : m_vertices[cur_idx].adj_dsts) {
                    if (!visited[dest_idx]) {
                        path[dest_idx].push_back(cur_idx);
                        nodes_to_mark_visited.insert(dest_idx);
                    }
                }
            }
            for (const auto& i : nodes_to_mark_visited) {
                visited[i] = true;
                search_queue.push(i);
            }
        }
        if (!visited[word_end_idx]) {
            // Routes not found
            return {};
        }
        // Generate routes
        std::vector<std::vector<std::string>> result;
        std::vector<std::pair<size_t, size_t>> traversal_stack;
        traversal_stack.emplace_back(word_end_idx, 0);
        while (!traversal_stack.empty()) {
            auto [word_idx, word_path_idx] = traversal_stack.back(); traversal_stack.pop_back();
            if (path[word_idx].size() == 0) {
                // Found start, add current route to result
                result.emplace_back();
                auto& cur_result = result.back();
                cur_result.emplace_back(word_start);
                for (const auto& [i_word_idx, i_word_path_idx] : traversal_stack | std::views::reverse) {
                    if (m_vertices[i_word_idx].is_special) {
                        // Skip auxiliary words
                        continue;
                    }
                    cur_result.emplace_back(m_vertices[i_word_idx].word);
                }
                continue;
            }
            if (word_path_idx == path[word_idx].size()) {
                // Fully iterated current word path; do nothing
            }
            else {
                // Continue iteration of current word path
                traversal_stack.emplace_back(word_idx, word_path_idx + 1);
                traversal_stack.emplace_back(path[word_idx][word_path_idx], 0);
            }
        }
        return result;
    }

private:
    struct Node {
        std::string word;
        std::vector<size_t> adj_dsts;
        bool is_special;
    };

    size_t find_word(std::string_view word) const {
        // auto it = std::find_if(m_vertices.begin(), m_vertices.end(),
        //     [&](const Node& v) { return v.word == word; });
        // return it - m_vertices.begin();
        auto it = m_word_idx_map.find(std::string(word));
        if (it == m_word_idx_map.end()) {
            return m_vertices.size();
        }
        return it->second;
    }

    // NOTE: Key must not use std::string_view, as SSO can cause
    //       string references to be invalidated
    std::unordered_map<std::string, size_t> m_word_idx_map;
    std::vector<Node> m_vertices;
};

int main(void) try {
    WordLadderGraph graph;
    {
        fmt_println("正在加载文件，请稍后...");
        std::ifstream fs("words.txt");
        if (!fs) {
            throw std::runtime_error("文件无效");
        }
        while (fs) {
            std::string line;
            getline(fs, line);
            if (line == "") { continue; }
            to_lower_str_inplace(line);
            graph.add_word(line);
        }
        fmt_println("加载完成。");
    }
    while (true) {
        std::string input;
        std::string_view word1 = "", word2 = "";
        fmt_print("输入两个单词(用空格分隔，键入 Ctrl+C 以中止): ");
        if (!getline(std::cin, input)) {
            break;
        }
        to_lower_str_inplace(input);
        // Input part
        try {
            size_t cur_rpos = 0, last_rpos = 0;
            skip_whitespace(input, cur_rpos);
            last_rpos = cur_rpos;
            while (!std::isspace(look_ahead(input, cur_rpos))) {
                cur_rpos++;
            }
            word1 = std::string_view{ input }.substr(last_rpos, cur_rpos - last_rpos);
            skip_whitespace(input, cur_rpos);
            last_rpos = cur_rpos;
            while (auto och = try_look_ahead(input, cur_rpos)) {
                if (std::isspace(*och)) { break; }
                cur_rpos++;
            }
            word2 = std::string_view{ input }.substr(last_rpos, cur_rpos - last_rpos);
            if (word1.size() != word2.size()) {
                fmt_println("错误: 输入的两个单词不等长");
                continue;
            }
        }
        catch (...) {
            fmt_println("错误：输入无效。请重试。");
            continue;
        }
        // Search part
        try {
            fmt_println("`{}` -> `{}` 的最短路径搜索结果:", word1, word2);
            auto result = graph.find_route(word1, word2);
            if (result.empty()) {
                fmt_println("错误: 不存在可实现转换的路径");
                continue;
            }
            if (!graph.word_in_graph(word1)) {
                fmt_println("* 起始单词不在列表内。请注意结果包括了起始单词不在列表内的情况。");
            }
            for (const auto& i : result) {
                fmt_print("{}", i[0]);
                for (const auto& word : i | std::views::drop(1)) {
                    fmt_print("->{}", word);
                }
                endl(std::cout);
            }
            fmt_println("概要: 共存在 {} 条路径, 最短路径长度为 {}", result.size(), result[0].size());
        }
        catch (const std::exception& e) {
            fmt_println("错误: {}", e.what());
            continue;
        }
    }
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
