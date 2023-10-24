#include <unordered_map>
#include <iostream>
#include <optional>
#include <fstream>
#include <format>
#include <vector>
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

struct BusGraph {
    void add_vertex(std::string_view site) {
        m_vertices.push_back({ std::string(site), {} });
    }
    // NOTE: If sites cannot be found, they will be automatically added
    void add_edge(std::string_view route, std::string_view site1, std::string_view site2) {
        auto find_or_add_vex_fn = [this](std::string_view site) {
            auto it = std::find_if(m_vertices.begin(), m_vertices.end(),
                [&](const BusGraphVertex& v) { return v.site == site; });
            if (it == m_vertices.end()) {
                m_vertices.push_back({ std::string(site), {} });
                it = m_vertices.end() - 1;
            }
            return it - m_vertices.begin();
        };
        auto idx1 = find_or_add_vex_fn(site1), idx2 = find_or_add_vex_fn(site2);
        m_vertices[idx1].adj_edges.emplace_back(m_edges.size());
        m_edges.emplace_back(std::string(route), idx2);
        m_vertices[idx2].adj_edges.emplace_back(m_edges.size());
        m_edges.emplace_back(std::string(route), idx1);
    }

    std::vector<std::string> get_min_sites_path(std::string_view site1, std::string_view site2) const {
        // Dijkstra
        size_t vidx1 = find_vertex_with_name(site1);
        size_t vidx2 = find_vertex_with_name(site2);
        if (vidx1 == m_vertices.size() || vidx2 == m_vertices.size()) {
            throw std::runtime_error("给定的站点名无效");
        }
        std::vector<bool> visited(m_vertices.size(), false);
        std::vector<size_t> dis(m_vertices.size(), 0x3f3f3f3f);
        std::vector<size_t> path(m_vertices.size(), 0xffffffff);
        dis[vidx1] = 0;
        path[vidx1] = vidx1;
        for (size_t i = 1; i < m_vertices.size(); i++) {
            size_t u = 0xffffffff;
            size_t min_d = 0x3f3f3f3f;
            for (size_t j = 0; j < m_vertices.size(); j++) {
                if (!visited[j] && dis[j] < min_d) {
                    u = j;
                    min_d = dis[j];
                }
            }
            if (u == 0xffffffff) {
                throw std::runtime_error("站点间没有联通的路线");
            }
            if (u == vidx2) {
                // Early exit as the shortest path we need has already
                // been built
                break;
            }
            visited[u] = true;
            for (const auto& idx : m_vertices[u].adj_edges) {
                size_t v = m_edges[idx].dest, w = 1;
                if (dis[u] + w < dis[v]) {
                    dis[v] = dis[u] + w;
                    path[v] = u;
                }
            }
        }
        // Build path
        std::stack<size_t> paths_idx;
        size_t cur_vex = vidx2;
        paths_idx.push(cur_vex);
        while (path[cur_vex] != cur_vex) {
            cur_vex = path[cur_vex];
            paths_idx.push(cur_vex);
        }
        std::vector<std::string> result;
        cur_vex = paths_idx.top(); paths_idx.pop();
        result.emplace_back(m_vertices[cur_vex].site);
        while (!paths_idx.empty()) {
            size_t next_vex = paths_idx.top(); paths_idx.pop();
            // Find edge: cur_vex -> next_vex
            std::string routes_str;
            for (const auto& idx : m_vertices[cur_vex].adj_edges) {
                if (m_edges[idx].dest == next_vex) {
                    if (routes_str != "") {
                        routes_str += ',';
                    }
                    routes_str += m_edges[idx].route;
                }
            }
            result.emplace_back(std::move(routes_str));
            result.emplace_back(m_vertices[next_vex].site);
            cur_vex = next_vex;
        }
        return result;
    }
    std::vector<std::string> get_min_transfers_path(std::string_view site1, std::string_view site2) const {
        // BFS
        size_t vidx1 = find_vertex_with_name(site1);
        size_t vidx2 = find_vertex_with_name(site2);
        if (vidx1 == m_vertices.size() || vidx2 == m_vertices.size()) {
            throw std::runtime_error("给定的站点名无效");
        }
        std::vector<bool> visited(m_vertices.size(), false);
        std::vector<size_t> path(m_vertices.size(), 0xffffffff);
        std::queue<std::pair<std::string_view, size_t>> routes_queue;
        std::unordered_map<std::string_view, bool> routes_visited;
        auto try_add_route_to_queue_fn = [&](std::string_view route_name, size_t origin) {
            if (!routes_visited[route_name]) {
                routes_visited[route_name] = true;
                routes_queue.emplace(route_name, origin);
            }
        };
        visited[vidx1] = true;
        path[vidx1] = vidx1;
        for (const auto& idx : m_vertices[vidx1].adj_edges) {
            try_add_route_to_queue_fn(m_edges[idx].route, vidx1);
        }
        while (!routes_queue.empty()) {
            if (visited[vidx2]) {
                // Early exit
                break;
            }
            size_t iter_cnt = routes_queue.size();
            for (size_t i = 0; i < iter_cnt; i++) {
                auto [route_name, origin] = routes_queue.front(); routes_queue.pop();
                for (const auto& edge : m_edges) {
                    if (edge.route != route_name) { continue; }
                    if (!visited[edge.dest]) {
                        visited[edge.dest] = true;
                        path[edge.dest] = origin;
                        for (const auto& idx : m_vertices[edge.dest].adj_edges) {
                            try_add_route_to_queue_fn(m_edges[idx].route, edge.dest);
                        }
                    }
                }
            }
        }
        if (!visited[vidx2]) {
            throw std::runtime_error("站点间没有联通的路线");
        }
        // Build path
        std::fill(visited.begin(), visited.end(), false);
        std::stack<size_t> paths_idx;
        size_t cur_vex = vidx2;
        paths_idx.push(cur_vex);
        while (path[cur_vex] != cur_vex) {
            cur_vex = path[cur_vex];
            paths_idx.push(cur_vex);
        }
        std::vector<std::string> result;
        auto add_seg_path_to_result_fn = [&](size_t v1, size_t v2) {
            // Add path (v1 -> v2, not including v1 itself) to result
            std::stack<size_t> traversed;
            visited[v2] = true;
            // Search in each route starting from v2
            for (const auto& idx : m_vertices[v2].adj_edges) {
                std::string_view route_name = m_edges[idx].route;
                size_t dest = m_edges[idx].dest;
                visited[dest] = true;
                traversed.push(dest);
                bool should_continue = true;
                // Deep search until found or failed
                while (traversed.top() != v1 && should_continue) {
                    should_continue = false;
                    for (const auto& idx : m_vertices[traversed.top()].adj_edges) {
                        size_t dest = m_edges[idx].dest;
                        if (!visited[dest] && m_edges[idx].route == route_name) {
                            visited[dest] = true;
                            traversed.push(dest);
                            should_continue = true;
                            break;
                        }
                    }
                }
                if (traversed.top() == v1) {
                    // Found
                    traversed.pop();
                    while (!traversed.empty()) {
                        size_t cur_idx = traversed.top(); traversed.pop();
                        visited[cur_idx] = false;
                        result.emplace_back(route_name);
                        result.emplace_back(m_vertices[cur_idx].site);
                    }
                    result.emplace_back(route_name);
                    result.emplace_back(m_vertices[v2].site);
                    // Early exit
                    break;
                }
                else {
                    // Failed
                    while (!traversed.empty()) {
                        size_t cur_idx = traversed.top(); traversed.pop();
                        visited[cur_idx] = false;
                    }
                }
            }
            visited[v2] = false;
        };
        cur_vex = paths_idx.top(); paths_idx.pop();
        result.emplace_back(m_vertices[cur_vex].site);
        while (!paths_idx.empty()) {
            size_t next_vex = paths_idx.top(); paths_idx.pop();
            // Find sub path: cur_vex -> next_vex
            add_seg_path_to_result_fn(cur_vex, next_vex);
            cur_vex = next_vex;
        }
        return result;
    }

private:
    struct BusGraphEdge {
        std::string route;
        size_t dest;
    };
    struct BusGraphVertex {
        std::string site;
        std::vector<size_t> adj_edges;
    };

    size_t find_vertex_with_name(std::string_view site) const {
        auto it = std::find_if(m_vertices.begin(), m_vertices.end(),
            [&](const BusGraphVertex& v) { return v.site == site; });
        return it - m_vertices.begin();
    }

    std::vector<BusGraphVertex> m_vertices;
    std::vector<BusGraphEdge> m_edges;
};

int main(void) try {
    BusGraph graph;
    {
        fmt_println("正在加载文件，请稍后...");
        std::ifstream fs("南京公交线路.txt");
        if (!fs) {
            throw std::runtime_error("文件无效");
        }
        while (fs) {
            std::string line;
            getline(fs, line);
            if (line == "") { continue; }
            // Format: <RouteName><Spaces><CommaSeparatedSitesList>
            size_t cur_rpos = 0;
            while (!std::isspace(look_ahead(line, cur_rpos))) {
                cur_rpos++;
            }
            std::string_view route_name = std::string_view{ line }.substr(0, cur_rpos);
            skip_whitespace(line, cur_rpos);
            size_t last_rpos = cur_rpos;
            std::string_view site1 = "", site2 = "";
            auto iterate_fn = [&]() {
                site1 = site2;
                site2 = std::string_view{ line }.substr(last_rpos, cur_rpos - last_rpos);
                if (site1 != "") {
                    graph.add_edge(route_name, site1, site2);
                }
            };
            while (auto och = try_look_ahead(line, cur_rpos)) {
                char ch = *och;
                if (ch == ',') {
                    iterate_fn();
                    last_rpos = cur_rpos + 1;
                }
                cur_rpos++;
            }
            iterate_fn();
        }
        fmt_println("加载完成。");
    }
    while (true) {
        std::string input;
        std::string_view site1 = "", site2 = "";
        fmt_print("输入两个站点名称(用空格分隔，键入 Ctrl+C 以中止): ");
        if (!getline(std::cin, input)) {
            break;
        }
        // Input part
        try {
            size_t cur_rpos = 0, last_rpos = 0;
            skip_whitespace(input, cur_rpos);
            last_rpos = cur_rpos;
            while (!std::isspace(look_ahead(input, cur_rpos))) {
                cur_rpos++;
            }
            site1 = std::string_view{ input }.substr(last_rpos, cur_rpos - last_rpos);
            skip_whitespace(input, cur_rpos);
            last_rpos = cur_rpos;
            while (auto och = try_look_ahead(input, cur_rpos)) {
                if (std::isspace(*och)) { break; }
                cur_rpos++;
            }
            site2 = std::string_view{ input }.substr(last_rpos, cur_rpos - last_rpos);
        }
        catch (...) {
            fmt_println("错误：输入无效。请重试。");
            continue;
        }
        // Search part
        try {
            auto print_path_vec_fn = [](const std::vector<std::string>& vec) {
                for (size_t i = 0; i < vec.size(); i++) {
                    if (i & 1) {
                        // Route
                        fmt_print("--({})->", vec[i]);
                    }
                    else {
                        // Site
                        fmt_print("{}", vec[i]);
                    }
                }
            };
            fmt_println("{} -> {} 的转车次数最少的乘车路线:", site1, site2);
            print_path_vec_fn(graph.get_min_transfers_path(site1, site2));
            endl(std::cout);
            fmt_println("{} -> {} 的经过站点最少的乘车路线:", site1, site2);
            print_path_vec_fn(graph.get_min_sites_path(site1, site2));
            endl(std::cout);
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
