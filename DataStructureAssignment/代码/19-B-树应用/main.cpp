#include <iostream>
#include <optional>
#include <fstream>
#include <format>
#include <ranges>
#include <queue>
#include <stack>
#include <span>

#include "json.h"
#include "hashmap.h"

#define fmt_print(...) (::std::cout << ::std::format(__VA_ARGS__))
#define fmt_println(...) (::std::cout << ::std::format(__VA_ARGS__) << ::std::endl)
#define fmt_write(fs, ...) ((fs) << ::std::format(__VA_ARGS__))
#define fmt_writeln(fs, ...) ((fs) << ::std::format(__VA_ARGS__) << ::std::endl)

template<typename T>
void shrink_vec(std::vector<T>& vec, size_t new_size) {
    if (vec.size() <= new_size) { return; }
    vec.erase(vec.cbegin() + new_size, vec.cend());
    // for (size_t i = new_size; i < orig_size; i++) {
    //     vec.pop_back();
    // }
}

// NOTE: This is not BTreeMap, so there exist only keys.
template<typename Key>
struct BTree {
    struct BTreeFindResult {
        bool found;
        Key key;
        std::vector<size_t> path;   // Children indices
    };

    BTree(size_t order) : m(order), m_root(nullptr) {
        if (order <= 2) { throw std::runtime_error("Invalid order for B-Tree"); }
    }
    BTree(const BTree& other) : m(other.m), m_root(nullptr) {
        if (other.m_root) {
            m_root = new BTreeNode(*other.m_root);
        }
    }
    BTree(BTree&& other) : m(other.m), m_root(other.m_root) {
        other.m_root = nullptr;
    }
    ~BTree() {
        if (m_root) {
            delete m_root;
        }
    }

    // If a new key is inserted, returns true
    bool insert_key(Key key) {
        check_integrity();
        if (!m_root) {
            m_root = new BTreeNode(m);
            m_root->keys.push_back(std::move(key));
            return true;
        }
        bool inserted;
        if (auto result = insert_key_recursive(*m_root, key, &inserted)) {
            // Create new root to accommodate new keys and children
            auto& [mval, rnode] = *result;
            BTreeNode new_root(m);
            new_root.keys.push_back(std::move(mval));
            new_root.children.push_back(std::move(*m_root));
            new_root.children.push_back(std::move(rnode));
            *m_root = std::move(new_root);
        }
        return inserted;
    }
    BTreeFindResult find_key(const Key& key) const {
        check_integrity();
        BTreeFindResult result{ .found = false, .key = key };
        size_t i;
        BTreeNode* ncur = m_root;
        while (ncur) {
            for (i = 0; i < ncur->keys.size(); i++) {
                if (ncur->keys[i] >= key) {
                    if (ncur->keys[i] == key) {
                        // Stop searching
                        result.found = true;
                        ncur = nullptr;
                    }
                    break;
                }
            }
            if (!result.found) {
                // Find in children
                ncur = ncur->is_leaf() ? nullptr : &ncur->children[i];
            }
            result.path.push_back(i);
        }
        return result;
    }
    // If a key is removed, returns true
    bool delete_key(const Key& key) {
        check_integrity();
        if (!m_root) { return false; }
        auto orphan_elem = delete_key_recursive(*m_root, key);
        if (orphan_elem && m_root->keys.size() < 1) {
            // Remove empty root
            if (m_root->is_leaf()) {
                delete m_root;
                m_root = nullptr;
            }
            else {
                auto temp_child = std::move(m_root->children[0]);
                *m_root = std::move(temp_child);
            }
        }
        return orphan_elem.has_value();
    }

    void check_integrity(void) const {
        if (!m_root) { return; }
        struct Checker {
            void operator()(const Key* pkey) {
                if (m_pkey) {
                    if (*m_pkey >= *pkey) {
                        throw std::runtime_error("B-tree was corrpted");
                    }
                }
                m_pkey = pkey;
            }
            private:
            const Key* m_pkey{};
        };
        Checker checker{};
        check_integrity_inner(*m_root, checker);
    }

    json::JsonObject to_json(void) const {
        json::JsonObject jo;
        jo["order"] = m;
        if (m_root) {
            jo["root"] = m_root->to_json();
        }
        else {
            jo["root"] = nullptr;
        }
        return jo;
    }
    static BTree from_json(const json::JsonObject& jo) {
        auto order = jo.at("order").get_value<size_t>();
        BTree result(order);
        const auto& jv_root = jo.at("root");
        if (!jv_root.is_null()) {
            result.m_root = new BTreeNode(BTreeNode::from_json(jv_root.get<json::JsonObject>(), order));
        }
        return result;
    }

private:
    const size_t m;     // Order of tree
    struct BTreeNode {
        BTreeNode(size_t order) {
            keys.reserve(order - 1);
            // children.reserve(order);
        }
        std::vector<Key> keys;
        std::vector<BTreeNode> children;
        bool is_leaf(void) const { return children.empty(); }
        json::JsonObject to_json(void) const {
            json::JsonArray ja_keys, ja_children;
            json::JsonObject jo;
            for (const auto& i : keys) {
                ja_keys.push_back(i);
            }
            if (!is_leaf()) {
                for (const auto& i : children) {
                    ja_children.push_back(i.to_json());
                }
            }
            jo["keys"] = std::move(ja_keys);
            jo["children"] = std::move(ja_children);
            return jo;
        }
        static BTreeNode from_json(const json::JsonObject& jo, size_t order) {
            BTreeNode result(order);
            for (const auto& i : jo.at("keys").get<json::JsonArray>()) {
                result.keys.push_back(i.get_value<Key>());
            }
            for (const auto& i : jo.at("children").get<json::JsonArray>()) {
                result.children.push_back(from_json(i.get<json::JsonObject>(), order));
            }
            return result;
        }
    };

    BTree(size_t order, BTreeNode* root) : m(order), m_root(root) {}

    template<typename Checker>
    void check_integrity_inner(const BTreeNode& node, Checker& checker) const {
        if (node.is_leaf()) {
            for (const auto& i : node.keys) {
                checker(&i);
            }
        }
        else {
            size_t i;
            for (i = 0; i < node.keys.size(); i++) {
                check_integrity_inner(node.children[i], checker);
                checker(&node.keys[i]);
            }
            check_integrity_inner(node.children[i], checker);
        }
    }

    // NOTE: If the insertion caused current node to split, the middle key and
    //       the right part will be returned
    std::optional<std::pair<Key, BTreeNode>> insert_key_recursive(BTreeNode& node, Key key, bool* pinserted) {
        size_t i;
        if (pinserted) { *pinserted = true; }
        for (i = 0; i < node.keys.size(); i++) {
            if (node.keys[i] >= key) {
                if (node.keys[i] == key) {
                    // Key already exists; do nothing
                    if (pinserted) { *pinserted = false; }
                    return std::nullopt;
                }
                break;
            }
        }
        // Key not found at current level; go ahead
        size_t midpoint = (m - 1 + 1 - 1) / 2;  // Prefer left
        std::optional<std::pair<Key, BTreeNode>> result;
        if (node.is_leaf()) {
            // Insert key into current node
            if (node.keys.size() == m - 1) {
                // Split full node
                std::optional<Key> midval;  // For delayed initialization
                BTreeNode right_node(m);
                if (i < midpoint) {
                    midval = std::move(node.keys[midpoint - 1]);
                    for (size_t j = midpoint + 1; j < m - 1 + 1; j++) {
                        right_node.keys.push_back(std::move(node.keys[j - 1]));
                    }
                    shrink_vec(node.keys, midpoint - 1);
                    node.keys.insert(node.keys.begin() + i, std::move(key));
                }
                else if (i == midpoint) {
                    midval = std::move(key);
                    for (size_t j = midpoint + 1; j < m - 1 + 1; j++) {
                        right_node.keys.push_back(std::move(node.keys[j - 1]));
                    }
                    shrink_vec(node.keys, midpoint);
                }
                else {  // i > midpoint
                    midval = std::move(node.keys[midpoint]);
                    for (size_t j = midpoint + 1; j < i; j++) {
                        right_node.keys.push_back(std::move(node.keys[j]));
                    }
                    right_node.keys.push_back(std::move(key));
                    for (size_t j = i + 1; j < m - 1 + 1; j++) {
                        right_node.keys.push_back(std::move(node.keys[j - 1]));
                    }
                    shrink_vec(node.keys, midpoint);
                }
                result = { std::move(*midval), std::move(right_node) };
            }
            else {
                // Insert nonfull
                node.keys.insert(node.keys.begin() + i, std::move(key));
            }
        }
        else if (auto insert_result = insert_key_recursive(node.children[i], std::move(key), pinserted)) {
            auto& [mval, rnode] = *insert_result;
            // Merge insertion result from children
            if (node.keys.size() == m - 1) {
                // Split full node
                std::optional<Key> midval;  // For delayed initialization
                BTreeNode right_node(m);
                if (i < midpoint) {
                    midval = std::move(node.keys[midpoint - 1]);
                    for (size_t j = midpoint + 1; j < m - 1 + 1; j++) {
                        right_node.keys.push_back(std::move(node.keys[j - 1]));
                    }
                    shrink_vec(node.keys, midpoint - 1);
                    node.keys.insert(node.keys.begin() + i, std::move(mval));
                    for (size_t j = midpoint + 1; j < m + 1; j++) {
                        right_node.children.push_back(std::move(node.children[j - 1]));
                    }
                    shrink_vec(node.children, midpoint);
                    node.children.insert(node.children.begin() + (i + 1), std::move(rnode));
                }
                else if (i == midpoint) {
                    midval = std::move(mval);
                    for (size_t j = midpoint + 1; j < m - 1 + 1; j++) {
                        right_node.keys.push_back(std::move(node.keys[j - 1]));
                    }
                    shrink_vec(node.keys, midpoint);
                    right_node.children.push_back(std::move(rnode));
                    for (size_t j = midpoint + 2; j < m + 1; j++) {
                        right_node.children.push_back(std::move(node.children[j - 1]));
                    }
                    shrink_vec(node.children, midpoint + 1);
                }
                else {  // i > midpoint
                    midval = std::move(node.keys[midpoint]);
                    for (size_t j = midpoint + 1; j < i; j++) {
                        right_node.keys.push_back(std::move(node.keys[j]));
                    }
                    right_node.keys.push_back(std::move(mval));
                    for (size_t j = i + 1; j < m - 1 + 1; j++) {
                        right_node.keys.push_back(std::move(node.keys[j - 1]));
                    }
                    shrink_vec(node.keys, midpoint);
                    for (size_t j = midpoint + 1; j < i + 1; j++) {
                        right_node.children.push_back(std::move(node.children[j]));
                    }
                    right_node.children.push_back(std::move(rnode));
                    for (size_t j = i + 2; j < m + 1; j++) {
                        right_node.children.push_back(std::move(node.children[j - 1]));
                    }
                    shrink_vec(node.children, midpoint + 1);
                }
                result = { std::move(*midval), std::move(right_node) };
            }
            else {
                // Insert nonfull
                node.keys.insert(node.keys.begin() + i, std::move(mval));
                node.children.insert(node.children.begin() + (i + 1), std::move(rnode));
            }
        }
        return result;
    }

    std::optional<Key> delete_key_recursive(BTreeNode& node, const Key& key) {
        size_t i;
        bool found = false;
        for (i = 0; i < node.keys.size(); i++) {
            if (node.keys[i] >= key) {
                if (node.keys[i] == key) {
                    // Found key; delete it
                    found = true;
                }
                break;
            }
        }
        if (node.is_leaf()) {
            // Delete or do nothing
            if (!found) { return std::nullopt; }
            auto elem = std::move(node.keys[i]);
            node.keys.erase(node.keys.begin() + i);
            return std::move(elem);
        }
        // Not leaves, go on
        std::optional<Key> elem;
        if (found) {
            // Move the previous greatest element up
            BTreeNode* ncur = &node.children[i];
            while (!ncur->is_leaf()) {
                ncur = &ncur->children.back();
            }
            elem = std::move(node.keys[i]);
            node.keys[i] = *delete_key_recursive(node.children[i], ncur->keys.back());
        }
        else {
            // Find in children
            elem = delete_key_recursive(node.children[i], key);
        }
        // Re-adjust nodes if necessary
        const size_t threshold = (m + 1) / 2 - 1;
        if (elem && node.children[i].keys.size() < threshold) {
            // Deletion brought children under the minimum size
            // Check left brother
            if (i > 0 && node.children[i - 1].keys.size() > threshold) {
                node.children[i].keys.insert(node.children[i].keys.begin(), std::move(node.keys[i - 1]));
                node.keys[i - 1] = std::move(node.children[i - 1].keys.back());
                node.children[i - 1].keys.pop_back();
                if (!node.children[i].is_leaf()) {
                    node.children[i].children.insert(node.children[i].children.begin(),
                        std::move(node.children[i - 1].children.back()));
                    node.children[i - 1].children.pop_back();
                }
            }
            // Check right brother
            else if (i + 1 < node.children.size() && node.children[i + 1].keys.size() > threshold) {
                node.children[i].keys.push_back(std::move(node.keys[i]));
                node.keys[i] = std::move(node.children[i + 1].keys.front());
                node.children[i + 1].keys.erase(node.children[i + 1].keys.begin());
                if (!node.children[i].is_leaf()) {
                    node.children[i].children.push_back(
                        std::move(node.children[i + 1].children.front()));
                    node.children[i + 1].children.erase(node.children[i + 1].children.begin());
                }
            }
            // No spare elements from brothers, perform merge
            else {
                if (i > 0) {
                    // Merge with left
                    node.children[i - 1].keys.push_back(std::move(node.keys[i - 1]));
                    for (auto& key : node.children[i].keys) {
                        node.children[i - 1].keys.push_back(std::move(key));
                    }
                    for (auto& child : node.children[i].children) {
                        node.children[i - 1].children.push_back(std::move(child));
                    }
                    node.keys.erase(node.keys.begin() + (i - 1));
                    node.children.erase(node.children.begin() + i);
                }
                else if (i + 1 < node.children.size()) {
                    // Merge with right
                    node.children[i].keys.push_back(std::move(node.keys[i]));
                    for (auto& key : node.children[i + 1].keys) {
                        node.children[i].keys.push_back(std::move(key));
                    }
                    for (auto& child : node.children[i + 1].children) {
                        node.children[i].children.push_back(std::move(child));
                    }
                    node.keys.erase(node.keys.begin() + i);
                    node.children.erase(node.children.begin() + (i + 1));
                }
                else {
                    throw std::runtime_error("Unreachable");
                }
            }
        }
        return elem;
    }

    BTreeNode* m_root;
};

std::vector<std::string> parse_command_line(std::string_view str) {
    std::vector<std::string> result;
    std::string cur_param = "";
    bool in_quotes = false, last_is_quote = false;
    auto iterate_fn = [&](char ch) {
        if (in_quotes) {
            if (ch == '"') {
                in_quotes = false;
            }
            else {
                cur_param += ch;
            }
        }
        else {
            if (ch == '"') {
                in_quotes = true;
                if (last_is_quote) {
                    // Convert double quotes to a single escaped one
                    cur_param += '"';
                }
            }
            else if (ch == ' ') {
                // Found end mark, commit current parameter
                if (cur_param != "") {
                    result.emplace_back(std::move(cur_param));
                    cur_param.clear();
                }
            }
            else {
                cur_param += ch;
            }
        }
        last_is_quote = (ch == '"');
    };
    for (auto ch : str) {
        iterate_fn(ch);
    }
    if (in_quotes) {
        // Close orphan quote
        in_quotes = false;
    }
    iterate_fn(' ');
    return result;
}

struct AppEnv {
    // value(tree_name, (file_path, BTree))
    hashmap::HashMap<std::string, std::pair<std::string, BTree<uint64_t>>> btrees;
    std::string active_btree_key;
    bool has_active_btree(void) const { return active_btree_key != ""; }
    void check_active_btree(void) const {
        if (!has_active_btree()) {
            throw std::runtime_error("未选中任何树");
        }
    }
    auto& get_active_btree_data(void) {
        check_active_btree();
        return btrees.at(active_btree_key);
    }
    std::string get_cmd_prompt(void) const {
        return std::format("({})", has_active_btree() ? active_btree_key : "N/A");
    }
};

void check_params_count(std::span<std::string> params, size_t count) {
    auto params_size = params.size() - 1;
    if (params_size != count) {
        throw std::runtime_error(std::format("命令不接受 {} 个参数(预期 {} 个)", params_size, count));
    }
}
void check_params_count(std::span<std::string> params, size_t low, size_t high) {
    if (low == high) { return check_params_count(params, low); }
    auto params_size = params.size() - 1;
    if (!(low <= params_size && params_size <= high)) {
        throw std::runtime_error(std::format("命令不接受 {} 个参数(预期 {}~{} 个)", params_size, low, high));
    }
}
double parse_f64_from_str(std::string_view str) {
    // Utilize JSON parser to parse f64
    json::JsonValue jv_order;
    if (!jv_order.try_deserialize_from_utf8(str.data(), str.size())) {
        throw std::runtime_error("Cannot parse f64 from string");
    }
    if (!jv_order.is_number()) {
        throw std::runtime_error("Cannot parse f64 from string");
    }
    return jv_order.get_value<double>();
}
uint64_t parse_u64_from_str(std::string_view str) {
    auto val = parse_f64_from_str(str);
    if (val < 0 || val != std::rint(val)) {
        throw std::runtime_error("Cannot parse u64 from string");
    }
    return static_cast<uint64_t>(val);
}

// Creates a new B-tree in memory
// Params: 1 - tree name
// Params: 2 - tree order
void sub_new(AppEnv& env, std::span<std::string> params) {
    check_params_count(params, 2);
    auto tree_order = parse_u64_from_str(params[2]);
    if (env.btrees.contains(params[1])) {
        fmt_println("* 名为 `{}` 的树已存在, 将跳过创建。", params[1]);
        return;
    }
    fmt_println("* 正在创建名为 `{}` 的新 B-树(度={})...", params[1], tree_order);
    env.btrees.insert({ params[1], { "", { tree_order } } });
}
// Loads a B-tree from file
// Params: 1 - tree name
//         2 - file path
void sub_load(AppEnv& env, std::span<std::string> params) {
    check_params_count(params, 2);
    if (env.btrees.contains(params[1])) {
        fmt_println("* 名为 `{}` 的树已存在, 将跳过读取。", params[1]);
        return;
    }
    fmt_println("* 正在从 `{}` 读取名为 `{}` 的 B-树数据...", params[2], params[1]);
    std::ifstream fs(params[2]);
    fs.exceptions(std::ios::failbit);
    std::vector<char> data{ std::istreambuf_iterator(fs), std::istreambuf_iterator<char>() };
    auto jv = json::JsonValue();
    if (!jv.try_deserialize_from_utf8(data) || !jv.is_object()) {
        throw std::runtime_error("无法从文件中加载 B-树数据");
    }
    using BTreeType = decltype(env.btrees)::value_type::second_type::second_type;
    env.btrees.insert({ params[1], { params[2], BTreeType::from_json(jv.get<json::JsonObject>()) } });
}
// Unloads active B-tree in memory
// Params: None
void sub_unload(AppEnv& env, std::span<std::string> params) {
    check_params_count(params, 0);
    env.check_active_btree();
    env.btrees.erase(env.btrees.find(env.active_btree_key));
    env.active_btree_key = "";
}
// Selects a new active B-tree
// Params: 1 - tree name
void sub_select(AppEnv& env, std::span<std::string> params) {
    check_params_count(params, 1);
    if (!env.btrees.contains(params[1])) {
        throw std::runtime_error(std::format("找不到名为 `{}` 的树", params[1]));
    }
    env.active_btree_key = params[1];
}
// Writes active B-tree to file
// Params: 1 - file path [optional]
// NOTE: If path is never specified, save will fail. Otherwise,
//       new path will override any previously set values.
void sub_sync(AppEnv& env, std::span<std::string> params) {
    check_params_count(params, 0, 1);
    using BTreeType = decltype(env.btrees)::value_type::second_type::second_type;
    auto write_to_file_fn = [&env](
        const BTreeType& btree, const std::string& tree_name, const std::string& path)
    {
        fmt_println("* 正在向 `{}` 写入名为 `{}` 的 B-树数据...", path, tree_name);
        std::ofstream fs(path);
        fs.exceptions(std::ios::failbit);
        auto data = json::JsonValue{ btree.to_json() }.serialize_into_utf8();
        std::copy(data.begin(), data.end(), std::ostreambuf_iterator(fs));
    };
    if (params.size() == 1) {
        auto& [path, btree] = env.get_active_btree_data();
        write_to_file_fn(btree, env.active_btree_key, path);
    }
    else if (params.size() == 2) {
        auto& [path, btree] = env.get_active_btree_data();
        write_to_file_fn(btree, env.active_btree_key, params[1]);
        path = params[1];
    }
}
// Writes all B-tree to file
// Params: None
void sub_syncall(AppEnv& env, std::span<std::string> params) {
    check_params_count(params, 0);
    using BTreeType = decltype(env.btrees)::value_type::second_type::second_type;
    auto write_to_file_fn = [&env](
        const BTreeType& btree, const std::string& tree_name, const std::string& path)
    {
        fmt_println("* 正在向 `{}` 写入名为 `{}` 的 B-树数据...", path, tree_name);
        std::ofstream fs(path);
        fs.exceptions(std::ios::failbit);
        auto data = json::JsonValue{ btree.to_json() }.serialize_into_utf8();
        std::copy(data.begin(), data.end(), std::ostreambuf_iterator(fs));
    };
    for (const auto& i : env.btrees) {
        if (i.second.first == "") { continue; }
        write_to_file_fn(i.second.second, i.first, i.second.first);
    }
}
// Dumps active B-tree as JSON to stdout
// Params: None
void sub_dump(AppEnv& env, std::span<std::string> params) {
    check_params_count(params, 0);
    json::JsonValue jv = env.get_active_btree_data().second.to_json();
    auto vec_chars = jv.serialize_into_utf8();
    fmt_println("{}", std::string_view(vec_chars.begin(), vec_chars.end()));
}
// Inserts new value to active B-tree
// Params: 1 - value
void sub_insert(AppEnv& env, std::span<std::string> params) {
    check_params_count(params, 1);
    auto& btree = env.get_active_btree_data().second;
    auto val = parse_u64_from_str(params[1]);
    if (btree.insert_key(val)) {
        fmt_println("* 向 `{}` 中插入了 `{}`。", env.active_btree_key, val);
    }
    else {
        fmt_println("* 未向 `{}` 中插入 `{}`(元素已存在)。", env.active_btree_key, val);
    }
}
// Finds value in active B-tree and prints metrics if found
// Params: 1 - value
void sub_find(AppEnv& env, std::span<std::string> params) {
    check_params_count(params, 1);
    auto& btree = env.get_active_btree_data().second;
    auto val = parse_u64_from_str(params[1]);
    auto find_result = btree.find_key(val);
    if (!find_result.found) {
        fmt_println("无法在 `{}` 中找到 `{}`。", env.active_btree_key, val);
    }
    else {
        std::string path_str;
        path_str = std::format("{}", find_result.path.front());
        for (const auto& i : find_result.path | std::views::drop(1)) {
            path_str += std::format("->{}", i);
        }
        fmt_println("在 `{}` 中找到了 `{}`(路径: {})。", env.active_btree_key, val, path_str);
    }
}
// Deletes value in active B-tree
// Params: 1 - value
void sub_delete(AppEnv& env, std::span<std::string> params) {
    check_params_count(params, 1);
    auto& btree = env.get_active_btree_data().second;
    auto val = parse_u64_from_str(params[1]);
    if (btree.delete_key(val)) {
        fmt_println("* 从 `{}` 中删除了 `{}`。", env.active_btree_key, val);
    }
    else {
        fmt_println("* 未从 `{}` 中删除 `{}`(找不到元素)。", env.active_btree_key, val);
    }
}

int main(void) try {
    std::string cmdline;
    AppEnv app_env{};
    fmt_println("* 这是交互式控制台。输入命令来操作 B-树。");
    while (true) {
        fmt_print("{}> ", app_env.get_cmd_prompt());
        if (!getline(std::cin, cmdline)) { break; }
        auto result = parse_command_line(cmdline);
        if (result.empty()) { continue; }
        const auto& cmd = result.front();
        try {
            if (cmd == "exit") {
                std::vector<std::string> sub_params{ "syncall" };
                sub_syncall(app_env, sub_params);
                fmt_println("感谢使用。");
                break;
            }
            else if (cmd == "new" || cmd == "n") { sub_new(app_env, result); }
            else if (cmd == "load") { sub_load(app_env, result); }
            else if (cmd == "unload") { sub_unload(app_env, result); }
            else if (cmd == "select" || cmd == "s") { sub_select(app_env, result); }
            else if (cmd == "sync") { sub_sync(app_env, result); }
            else if (cmd == "syncall") { sub_syncall(app_env, result); }
            else if (cmd == "dump") { sub_dump(app_env, result); }
            else if (cmd == "insert" || cmd == "i") { sub_insert(app_env, result); }
            else if (cmd == "find" || cmd == "f") { sub_find(app_env, result); }
            else if (cmd == "delete" || cmd == "d") { sub_delete(app_env, result); }
            else {
                fmt_println("错误: 无法识别指令。请重试。");
            }
        }
        catch (const std::exception& e) {
            fmt_println("错误: 执行命令时抛出了异常: {}", e.what());
        }
        catch (...) {
            fmt_println("错误: 执行命令时抛出了未知异常");
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
