#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdio>
#include <format>
#include <random>

#include "hashmap.h"
#include "json.h"

void flush_stdin(void) {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int gen_random_number(int start, int end) {
    static std::mt19937 gen{ std::random_device{}() };
    return std::uniform_int_distribution<int>{ start, end }(gen);
}
std::string gen_random_string(void) {
    int loop_cnt = gen_random_number(3, 8);
    std::string result;
    for (int i = 0; i < loop_cnt; i++) {
        result += "abcdefghijklmnopqrstuvwxyz"[gen_random_number(0, 25)];
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

#define fmt_print(...) (::std::cout << ::std::format(__VA_ARGS__))
#define fmt_println(...) (::std::cout << ::std::format(__VA_ARGS__) << ::std::endl)
#define fmt_write(fs, ...) ((fs) << ::std::format(__VA_ARGS__))
#define fmt_writeln(fs, ...) ((fs) << ::std::format(__VA_ARGS__) << ::std::endl)

enum Size {
    Small = 1,
    Medium = 2,
    Large = 3,
};

#define MAX_PERSON 30

struct Package {
    int subid;
    Size size;
    std::string receiver_name;
    std::string receiver_phone;
    uint64_t added_ts;  // Unit: day
    std::string get_id(void) const {
        return std::to_string(size) + "-" + std::to_string(subid);
    }

    json::JsonObject to_json(void) const {
        json::JsonObject jo;
        jo["subid"] = subid;
        jo["size"] = size;
        jo["receiver_name"] = receiver_name;
        jo["receiver_phone"] = receiver_phone;
        jo["added_time"] = added_ts;
        return jo;
    }
    static Package from_json(const json::JsonObject& jo) {
        Package result;
        result.subid = jo.at("subid").get_value<int>();
        result.size = jo.at("size").get_value<Size>();
        result.receiver_name = jo.at("receiver_name").get_value<std::string>();
        result.receiver_phone = jo.at("receiver_phone").get_value<std::string>();
        result.added_ts = jo.at("added_time").get_value<uint64_t>();
        return result;
    }
};

struct Shelf {
    Shelf(Size pkgsize, size_t capacity) : m_head(new Node{}),
        m_pkgsize(pkgsize), m_capacity(capacity) {}
    Shelf(const Shelf& other) : m_head(new Node{}),
        m_pkgsize(other.m_pkgsize), m_capacity(other.m_capacity)
    {
        // Duplicate linked list
        Node* ndpre = m_head;
        const Node* nscur = other.m_head->next;
        while (nscur) {
            ndpre->next = new Node{ ndpre, nullptr, nscur->data };
            ndpre = ndpre->next;
        }
        ndpre->next = nullptr;
    }
    Shelf(Shelf&& other) : Shelf(other.m_pkgsize, other.m_capacity) {
        using std::swap;
        swap(*this, other);
    }
    Shelf& operator=(Shelf rhs) noexcept {
        using std::swap;
        swap(*this, rhs);
    }
    ~Shelf() {
        // Destroy the linked list
        while (m_head) {
            Node* next = m_head->next;
            delete m_head;
            m_head = next;
        }
    }
    bool is_full(void) {
        Node* ncur = m_head->next;
        for (size_t i = 0; i < m_capacity; i++) {
            if (!ncur) { return false; }
            ncur = ncur->next;
        }
        return true;
    }
    size_t get_count(void) {
        Node* ncur = m_head->next;
        size_t count;
        for (count = 0; ncur; count++) {
            ncur = ncur->next;
        }
        return count;
    }
    // NOTE: pkg.subid is ignored and recoded
    // NOTE: Upon success, returns pointer to package
    const Package* add_package(Package pkg) {
        // v[i]: Whether item of index i appeared
        bool *v = new bool[m_capacity]{};
        const Package* ppkg = nullptr;
        Node* ncur = m_head;
        // NOTE: We need to preserve tail pointer
        while (ncur->next) {
            v[ncur->next->data.subid - 1] = true;
            ncur = ncur->next;
        }
        for (size_t i = 0; i < m_capacity; i++) {
            if (!v[i]) {
                // Add package
                pkg.subid = static_cast<int>(i + 1);
                ncur->next = new Node{ ncur, nullptr, std::move(pkg) };
                ppkg = &ncur->next->data;
                break;
            }
        }
        delete[] v;
        return ppkg;
    }
    // NOTE: Functor: bool(const Package&); returns whether
    //       packages should be kept
    template<typename Functor>
    void retain_packages(Functor&& functor) {
        Node* ncur = m_head->next;
        while (ncur) {
            if (functor(ncur->data)) {
                // Do nothing
                ncur = ncur->next;
            }
            else {
                // Remove current node
                Node* npre = ncur->prev, *next = ncur->next;
                npre->next = next;
                if (next) {
                    next->prev = npre;
                }
                delete ncur;
                ncur = next;
            }
        }
    }
    void sort_packages(void) {
        m_head->next = merge_sort(m_head->next);
        // Extra: Fix back pointer
        if (m_head->next) {
            m_head->next->prev = m_head;
        }
    }

    json::JsonObject to_json(void) const {
        json::JsonObject jo;
        json::JsonArray ja_pkgs;
        jo["pkgsize"] = json::JsonValue(m_pkgsize);
        jo["capacity"] = m_capacity;
        Node* ncur = m_head->next;
        while (ncur) {
            ja_pkgs.push_back(ncur->data.to_json());
            ncur = ncur->next;
        }
        jo["packages"] = std::move(ja_pkgs);
        return jo;
    }
    static Shelf from_json(const json::JsonObject& jo) {
        Shelf result{
            jo.at("pkgsize").get_value<Size>(),
            jo.at("capacity").get_value<size_t>(),
        };
        Node* nhead = result.m_head;
        Node* npre = nhead;
        for (const auto& i : jo.at("packages").get<json::JsonArray>()) {
            npre->next = new Node{ npre, nullptr, Package::from_json(i.get<json::JsonObject>()) };
            npre = npre->next;
        }
        npre->next = nullptr;
        return result;
    }

    friend void swap(Shelf& a, Shelf& b) noexcept {
        using std::swap;
        swap(a.m_pkgsize, b.m_pkgsize);
        swap(a.m_capacity, b.m_capacity);
        swap(a.m_head, b.m_head);
    }

private:
    struct Node {
        Node *prev, *next;
        Package data;
    };

    Node* merge_sort(Node* head) {
        constexpr size_t SLOT_COUNT = 64;
        Node* slots[SLOT_COUNT]{};
        auto merge_fn = [&slots](Node* node, size_t slot_i) -> Node* {
            if (!slots[slot_i]) {
                // Add to slot
                slots[slot_i] = node;
                return nullptr;
            }
            // Merge and remove from slot
            Node *ndhead, **ndcur = &ndhead;
            Node *ncur1 = node, *ncur2 = slots[slot_i];
            slots[slot_i] = nullptr;
            while (ncur1 && ncur2) {
                if (ncur1->data.subid < ncur2->data.subid) {
                    *ndcur = ncur1;
                    ndcur = &(*ndcur)->next;
                    ncur1 = ncur1->next;
                }
                else {
                    *ndcur = ncur2;
                    ndcur = &(*ndcur)->next;
                    ncur2 = ncur2->next;
                }
                *ndcur = ncur1 ? ncur1 : ncur2;
            }
            return ndhead;
        };
        while (head) {
            Node *next = head->next;
            head->next = nullptr;
            size_t i = 0;
            Node* node = head;
            while ((node = merge_fn(node, i)) != nullptr) {
                i++;
            }
            head = next;
        }
        // Merge all slots
        Node* node = nullptr;
        for (size_t i = 0; i < SLOT_COUNT; i++) {
            if (slots[i]) {
                node = node ? merge_fn(node, i) : slots[i];
            }
        }
        // Extra: Fix back pointer (code above only designed for singly linked list)
        Node *npre = nullptr, *ncur = node;
        while (ncur) {
            ncur->prev = npre;
            npre = ncur;
            ncur = ncur->next;
        }
        return node;
    }

    Size m_pkgsize;
    size_t m_capacity;
    Node* m_head;
};

struct CaiNiaoSystem {
    CaiNiaoSystem(std::string storage_path) : m_storage_path(std::move(storage_path)),
        m_shelf_large(Large, 50), m_shelf_medium(Medium, 100), m_shelf_small(Small, 500),
        m_cur_ts(0), m_events()
    {
        if (!this->load_from_disk()) {
            fmt_println("错误: 无法加载文件。系统将以默认状态初始化。");
        }
    }
    CaiNiaoSystem(const CaiNiaoSystem&) = delete;
    ~CaiNiaoSystem() {
        if (!this->save_to_disk()) {
            fmt_println("错误: 无法保存文件");
        }
    }
    bool load_from_disk(void) {
        fmt_println("* 正在读盘(\"{}\")...", m_storage_path);
        std::ifstream fs(m_storage_path);
        std::vector<char> data{ std::istreambuf_iterator(fs), std::istreambuf_iterator<char>() };
        auto jv = json::JsonValue();
        if (!jv.try_deserialize_from_utf8(data)) {
            return false;
        }
        if (!jv.is_object()) {
            return false;
        }

        try {
            auto cur_ts = jv.at("current_time").get_value<uint64_t>();
            auto shelf_large = Shelf::from_json(jv.at("large_shelf").get<json::JsonObject>());
            auto shelf_medium = Shelf::from_json(jv.at("medium_shelf").get<json::JsonObject>());
            auto shelf_small = Shelf::from_json(jv.at("small_shelf").get<json::JsonObject>());
            std::vector<EventRecord> events;
            for (const auto& i : jv.at("events").get<json::JsonArray>()) {
                EventRecord ev;
                const std::string& kind_str = i.at("kind").get<std::string>();
                if (kind_str == "added") {
                    ev.kind = EventRecord::PackageAdded;
                }
                else if (kind_str == "discarded") {
                    ev.kind = EventRecord::PackageDiscarded;
                }
                else if (kind_str == "picked") {
                    ev.kind = EventRecord::PackagePicked;
                }
                else {
                    throw std::runtime_error("Unrecognized event type");
                }
                ev.pkg_info = Package::from_json(i.at("pkg_info").get<json::JsonObject>());
                ev.ts = i.at("time").get_value<uint64_t>();
                events.push_back(std::move(ev));
            }
            m_shelf_large.sort_packages();
            m_shelf_medium.sort_packages();
            m_shelf_small.sort_packages();
            using std::swap;
            // Actually update system data
            swap(m_cur_ts, cur_ts);
            swap(m_shelf_large, shelf_large);
            swap(m_shelf_medium, shelf_medium);
            swap(m_shelf_small, shelf_small);
            swap(m_events, events);
        }
        catch (...) {
            return false;
        }

        return true;
    }
    bool save_to_disk(void) {
        fmt_println("* 正在存盘(\"{}\")...", m_storage_path);
        std::ofstream fs(m_storage_path);
        json::JsonValue jv;
        jv = json::JsonObject();
        jv["current_time"] = m_cur_ts;
        jv["large_shelf"] = m_shelf_large.to_json();
        jv["medium_shelf"] = m_shelf_medium.to_json();
        jv["small_shelf"] = m_shelf_small.to_json();
        json::JsonArray ja_events;
        for (const auto& event : m_events) {
            json::JsonObject jo_event;
            const char* kind_str = "invalid";
            switch (event.kind) {
            case EventRecord::PackageAdded:         kind_str = "added";         break;
            case EventRecord::PackageDiscarded:     kind_str = "discarded";     break;
            case EventRecord::PackagePicked:        kind_str = "picked";        break;
            }
            jo_event["kind"] = kind_str;
            jo_event["pkg_info"] = event.pkg_info.to_json();
            jo_event["time"] = event.ts;
            ja_events.push_back(std::move(jo_event));
        }
        jv["events"] = std::move(ja_events);
        auto data = jv.serialize_into_utf8();
        std::copy(data.begin(), data.end(), std::ostreambuf_iterator(fs));
        return static_cast<bool>(fs);
    }

    // NOTE: pkg.added_ts will be reassigned with system date
    void add_package(Package pkg) {
        const Package* ppkg;
        Shelf* target_shelf;
        pkg.added_ts = m_cur_ts;
        switch (pkg.size) {
        case Large:
            fmt_println("* 正在将新的包裹添加到大货架...");
            target_shelf = &m_shelf_large;
            break;
        case Medium:
            fmt_println("* 正在将新的包裹添加到中货架...");
            target_shelf = &m_shelf_medium;
            break;
        case Small:
            fmt_println("* 正在将新的包裹添加到小货架...");
            target_shelf = &m_shelf_small;
            break;
        default:
            fmt_println("错误: 无法识别包裹的大小");
            return;
        }
        ppkg = target_shelf->add_package(std::move(pkg));
        if (!ppkg) {
            fmt_println("错误: 添加失败，也许货架已满？");
            return;
        }
        target_shelf->sort_packages();
        fmt_println("* 添加成功，包裹编号: {}，取件人: {}(手机号: {})",
            ppkg->get_id(), ppkg->receiver_name, ppkg->receiver_phone
        );
        m_events.emplace_back(EventRecord::PackageAdded, *ppkg, m_cur_ts);
    }
    void pick_package(std::string code_or_phone) {
        // NOTE: Receiver name is used to pick out all related packages
        std::string receiver_name;
        auto retain_find_fn = [&](const Package& pkg) {
            if (receiver_name != "") { return true; }
            if (code_or_phone == pkg.receiver_phone || code_or_phone == pkg.get_id()) {
                // Found package
                receiver_name = pkg.receiver_name;
            }
            return true;
        };
        Shelf* shelves[] = { &m_shelf_large, &m_shelf_medium, &m_shelf_small };
        for (auto p : shelves) {
            if (receiver_name != "") { break; }
            p->retain_packages(retain_find_fn);
        }
        if (receiver_name == "") {
            fmt_println("* 未能找到任何与所提供的 `{}` 关联的包裹。", code_or_phone);
            return;
        }
        auto retain_pick_fn = [&](const Package& pkg) {
            if (receiver_name == pkg.receiver_name) {
                fmt_println("* 找到了包裹 {}，取件人: {}(手机号: {})，已取出。",
                    pkg.get_id(), pkg.receiver_name, pkg.receiver_phone
                );
                m_events.emplace_back(EventRecord::PackagePicked, pkg, m_cur_ts);
                return false;
            }
            return true;
        };
        for (auto p : shelves) {
            p->retain_packages(retain_pick_fn);
        }
    }
    void advance_day(void) {
        m_cur_ts++;
        fmt_println("* 系统时间已变更为第 {} 天。", m_cur_ts);
        // Also discard expired packages
        auto retain_fn = [this](const Package& pkg) {
            if (m_cur_ts - pkg.added_ts > 2) {
                fmt_println("* 包裹 {} 存放时间过长(于第 {} 天入库)，已退回。",
                    pkg.get_id(), pkg.added_ts
                );
                m_events.emplace_back(EventRecord::PackageDiscarded, pkg, m_cur_ts);
                return false;
            }
            return true;
        };
        m_shelf_large.retain_packages(retain_fn);
        m_shelf_medium.retain_packages(retain_fn);
        m_shelf_small.retain_packages(retain_fn);
    }
    void print_sys_metrics(void) {
        fmt_println("* 正在统计系统中的 {} 条事件...", m_events.size());
        size_t added_pkgs_cnt = 0, picked_pkgs_cnt = 0, discarded_pkgs_cnt = 0;
        hashmap::HashMap<std::string, size_t> today_person_added_m;
        size_t week_discarded_pkgs_cnt = 0, today_added_pkgs_cnt = 0;
        for (size_t i = 0; i < m_events.size(); i++) {
            const auto& ev = m_events[i];
            switch (ev.kind) {
            case EventRecord::PackageAdded:
                added_pkgs_cnt++;
                if (m_cur_ts == ev.ts) {
                    today_person_added_m[ev.pkg_info.receiver_name]++;
                    today_added_pkgs_cnt++;
                }
                break;
            case EventRecord::PackageDiscarded:
                discarded_pkgs_cnt++;
                if (m_cur_ts - ev.ts < 7) {
                    week_discarded_pkgs_cnt++;
                }
                break;
            case EventRecord::PackagePicked:
                picked_pkgs_cnt++;
                break;
            default:
                fmt_println("警告: 发现了无法解析的事件，已忽略。");
                break;
            }
        }
        size_t max_person_added_pkg_cnt = 0;
        for (const auto& i : today_person_added_m) {
            if (i.second > max_person_added_pkg_cnt) {
                max_person_added_pkg_cnt = i.second;
            }
        }
        auto get_max_person_added_str = [&]() -> std::string {
            if (max_person_added_pkg_cnt == 0) {
                return "<不存在>";
            }
            std::string result;
            bool first = true;
            for (const auto& i : today_person_added_m) {
                if (i.second != max_person_added_pkg_cnt) { continue; }
                if (!first) {
                    result += ", ";
                }
                result += std::format("{}", i.first);
                first = false;
            }
            result += std::format(" ({} 个包裹)", max_person_added_pkg_cnt);
            return result;
        };
        fmt_println("统计信息:");
        fmt_println("    当前系统时间: 第 {} 天", m_cur_ts);
        fmt_println("    当前存放的包裹数: {}", added_pkgs_cnt - picked_pkgs_cnt - discarded_pkgs_cnt);
        fmt_println("    总计添加的包裹数: {}", added_pkgs_cnt);
        fmt_println("    总计逾期下架的包裹数: {}", discarded_pkgs_cnt);
        fmt_println("    总计正常取回的包裹数: {}", picked_pkgs_cnt);
        fmt_println("    最近一周内逾期被退回的包裹数: {}", week_discarded_pkgs_cnt);
        fmt_println("    当天收到包裹数最多的人: {}", get_max_person_added_str());
        fmt_println("    当天有包裹人的平均包裹数量: {} 个 / {} 人 ({:.2f} 个/人)",
            today_added_pkgs_cnt, today_person_added_m.size(),
            static_cast<double>(today_added_pkgs_cnt) / today_person_added_m.size()
        );
    }

private:
    std::string m_storage_path;
    uint64_t m_cur_ts;    // Unit: day
    Shelf m_shelf_large, m_shelf_medium, m_shelf_small;
    struct EventRecord {
        enum {
            Invalid = 0,
            PackageAdded,
            PackageDiscarded,
            PackagePicked,
        } kind;
        Package pkg_info;
        uint64_t ts;
    };
    std::vector<EventRecord> m_events;
};

void menu_loop(CaiNiaoSystem &system) {
    std::string tmpstr;
    auto input_choice_fn = []() {
        unsigned choice;
        while (true) {
            fmt_print("> ");
            if (std::cin >> choice) { break; }
            fmt_println("错误: 输入无效。请重试。");
            flush_stdin();
            continue;
        }
        flush_stdin();
        return choice;
    };
    auto save_fn = [&system]() {
        if (!system.save_to_disk()) {
            fmt_println("错误: 无法保存文件");
        }
    };
    auto sub_add_package_fn = [&system]() {
        Package pkg{};
        int tmpn;
        fmt_print("请输入包裹大小(1-小; 2-中; 3-大): ");
        if (!(std::cin >> tmpn) || tmpn < 1 || tmpn > 3) {
            fmt_println("错误: 包裹大小无效。添加过程被中止。");
            return;
        }
        pkg.size = static_cast<Size>(tmpn);
        fmt_print("请输入包裹取件人姓名: ");
        if (!(std::cin >> pkg.receiver_name)) {
            fmt_println("错误: 输入无效。添加过程被中止。");
            return;
        }
        fmt_print("请输入包裹取件人手机号: ");
        if (!(std::cin >> pkg.receiver_phone)) {
            fmt_println("错误: 输入无效。添加过程被中止。");
            return;
        }
        system.add_package(std::move(pkg));
    };
    // value(name, phone)
    std::vector<std::pair<std::string, std::string>> community_people;
    auto pick_random_person_fn = [&] {
        if (community_people.empty()) {
            fmt_println("* 尚未生成社区人员。正在随机生成...");
            community_people.reserve(MAX_PERSON);
            for (size_t i = 0; i < MAX_PERSON; i++) {
                std::string name = gen_random_string();
                std::string phone = std::to_string(gen_random_number(
                    static_cast<int>(1e9), static_cast<int>(1e9 + 1e8 + 1e7)
                ));
                community_people.emplace_back(std::move(name), std::move(phone));
            }
        }
        return community_people[gen_random_number(0, MAX_PERSON - 1)];
    };
    while (true) {
        fmt_println("-----选项菜单-----");
        fmt_println("1. 进行新的一天(并/或应用随机事件)");
        fmt_println("2. 手动包裹上架");
        fmt_println("3. 手动取件");
        fmt_println("4. 查看统计信息");
        fmt_println("5. 强制加载文件数据(将覆盖当前系统数据!)");
        fmt_println("0. 退出");
        auto choice = input_choice_fn();
        switch (choice) {
        case 0:
            fmt_println("感谢使用。");
            return;
        case 1:
            system.advance_day();
            if (input_yesno("要随机上架包裹吗?", true)) {
                int loop_cnt = gen_random_number(1, 5);
                for (int i = 0; i < loop_cnt; i++) {
                    int inner_loop_cnt = gen_random_number(1, 5);
                    auto [name, phone] = pick_random_person_fn();
                    for (int j = 0; j < inner_loop_cnt; j++) {
                        Package pkg{};
                        pkg.size = static_cast<Size>(gen_random_number(1, 3));
                        pkg.receiver_name = name;
                        pkg.receiver_phone = phone;
                        system.add_package(std::move(pkg));
                    }
                }
            }
            if (input_yesno("要随机取出包裹吗?", true)) {
                int loop_cnt = gen_random_number(1, 30);
                for (int i = 0; i < loop_cnt; i++) {
                    // std::string pkg_code = std::format("{}-{}",
                    //     gen_random_number(1, 3), gen_random_number(1, 500)
                    // );
                    // system.pick_package(std::move(pkg_code));
                    auto phone = pick_random_person_fn().second;
                    system.pick_package(std::move(phone));
                }
            }
            save_fn();
            break;
        case 2:
            sub_add_package_fn();
            save_fn();
            break;
        case 3:
            fmt_print("请输入取件码或取件人手机号: ");
            std::cin >> tmpstr;
            system.pick_package(tmpstr);
            save_fn();
            break;
        case 4:
            system.print_sys_metrics();
            break;
        case 5:
            if (!system.load_from_disk()) {
                fmt_println("错误: 无法加载文件");
            }
            break;
        default:
            fmt_println("错误: 输入的菜单项({})无效。请重试。", choice);
            break;
        }
    }
}

int main(void) try {
    fmt_println("菜鸟智慧系统");
    endl(std::cout);
    CaiNiaoSystem system("./cainiaosystem.txt");
    menu_loop(system);
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
