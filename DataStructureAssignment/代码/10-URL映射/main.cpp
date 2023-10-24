#include <functional>
#include <algorithm>
#include <iostream>
#include <string>
#include <cctype>

struct RequestHandler {
    using Handler = std::function<std::string(std::vector<std::string>)>;
    using Handler404 = std::function<std::string(void)>;

    RequestHandler() : m_handler_404([] { return "404 Not Found"; }) {}

    void mount(const std::string& rule, Handler handler) {
        m_handlers.emplace_back(RuleDesc::from_str(rule), std::move(handler));
    }
    void mount_404(Handler404 handler) {
        m_handler_404 = std::move(handler);
    }

    std::string handle_request(const std::string& path) {
        if (path.size() == 0) { return m_handler_404(); }
        for (auto& p : m_handlers) {
            auto& rule = p.first;
            auto& handler = p.second;
            if (path[0] != '/') { break; }
            size_t path_scan_idx = 1;
            size_t seg_idx = 0;
            std::vector<std::string> params;
            auto is_path_seg_end_fn = [&](size_t path_idx) {
                return path_idx == path.size() || path[path_idx] == '/';
            };
            for (seg_idx = 0; seg_idx < rule.segments.size(); seg_idx++) {
                const auto& cur_seg = rule.segments[seg_idx];
                if (cur_seg.type == cur_seg.ExactMatch) {
                    size_t in_seg_scan_idx = 0;
                    while (path_scan_idx < path.size() && in_seg_scan_idx < cur_seg.payload.size()) {
                        if (cur_seg.payload[in_seg_scan_idx] != path[path_scan_idx]) { break; }
                        in_seg_scan_idx++;
                        path_scan_idx++;
                    }
                    if (!(in_seg_scan_idx == cur_seg.payload.size() && is_path_seg_end_fn(path_scan_idx))) {
                        // Fail (not an exact match)
                        break;
                    }
                }
                else if (cur_seg.type == cur_seg.Integer) {
                    int v = 0;
                    if (is_path_seg_end_fn(path_scan_idx)) {
                        // Fail (forbid zero-length read)
                        break;
                    }
                    while (!is_path_seg_end_fn(path_scan_idx)) {
                        if (!std::isdigit(path[path_scan_idx])) { break; }
                        v = v * 10 + (path[path_scan_idx] - '0');
                        path_scan_idx++;
                    }
                    if (!is_path_seg_end_fn(path_scan_idx)) {
                        // Fail (found non-digit)
                        break;
                    }
                    params.emplace_back(std::to_string(v));
                }
                else if (cur_seg.type == cur_seg.String) {
                    std::string v;
                    if (is_path_seg_end_fn(path_scan_idx)) {
                        // Fail (forbid zero-length read)
                        break;
                    }
                    while (!is_path_seg_end_fn(path_scan_idx)) {
                        v += path[path_scan_idx];
                        path_scan_idx++;
                    }
                    params.emplace_back(v);
                }
                else if (cur_seg.type == cur_seg.Path) {
                    if (is_path_seg_end_fn(path_scan_idx)) {
                        // Fail (forbid zero-length read)
                        break;
                    }
                    params.emplace_back(path.substr(path_scan_idx));
                    path_scan_idx = path.size();
                }
                else {
                    throw std::runtime_error("Unknown match type");
                }
                // Skip slash
                if (path_scan_idx < path.size()) {
                    path_scan_idx++;
                }
            }
            if (seg_idx != rule.segments.size() || path_scan_idx != path.size()) {
                // Match failed
                continue;
            }
            // Extra: Check for trailing slash
            if (rule.should_check_trailing_slash && rule.has_trailing_slash != (path.back() == '/')) {
                // Match failed
                continue;
            }
            // Match succeeded
            return handler(std::move(params));
        }
        return m_handler_404();
    }

private:
    struct RuleDesc {
        struct PathSegment {
            enum {
                ExactMatch,
                Integer,
                String,
                Path,
            } type;
            std::string payload;
        };
        std::vector<PathSegment> segments;
        bool should_check_trailing_slash;
        bool has_trailing_slash;
        static RuleDesc from_str(const std::string& str) {
            RuleDesc result;
            result.should_check_trailing_slash = true;
            std::string cur_seg_str;
            auto commit_fn = [&]() {
                if (cur_seg_str != "") {
                    if (cur_seg_str == "<int>") {
                        result.segments.push_back({ PathSegment::Integer, "" });
                    }
                    else if (cur_seg_str == "<str>") {
                        result.segments.push_back({ PathSegment::String, "" });
                    }
                    else if (cur_seg_str == "<path>") {
                        result.segments.push_back({ PathSegment::Path, "" });
                        result.should_check_trailing_slash = false;
                    }
                    else {
                        result.segments.push_back({ PathSegment::ExactMatch, cur_seg_str });
                    }
                    cur_seg_str = "";
                }
            };
            for (auto ch : str) {
                if (ch == '/') {
                    commit_fn();
                    continue;
                }
                cur_seg_str += ch;
            }
            commit_fn();
            result.has_trailing_slash = str.back() == '/';
            return result;
        };
    };

    Handler404 m_handler_404;
    std::vector<std::pair<RuleDesc, Handler>> m_handlers;
};

int main(void) {
    RequestHandler handler;
    int n, m;
    std::cin >> n >> m;
    for (int i = 0; i < n; i++) {
        std::string rule, name;
        std::cin >> rule >> name;
        handler.mount(rule, [=](std::vector<std::string> params) {
            std::string response = name;
            for (const auto& param : params) {
                response += ' ';
                response += param;
            }
            return response;
        });
    }
    handler.mount_404([] { return "404"; });
    for (int i = 0; i < m; i++) {
        std::string input;
        std::cin >> input;
        std::cout << handler.handle_request(input) << std::endl;
    }
}
