#include <unordered_map>
#include <iostream>
#include <fstream>
#include <format>
#include <memory>
#include <ranges>
#include <queue>
#include <span>

#define fmt_print(...) (::std::cout << ::std::format(__VA_ARGS__))
#define fmt_println(...) (::std::cout << ::std::format(__VA_ARGS__) << ::std::endl)
#define fmt_write(fs, ...) ((fs) << ::std::format(__VA_ARGS__))
#define fmt_writeln(fs, ...) ((fs) << ::std::format(__VA_ARGS__) << ::std::endl)

std::string byte_size_to_str(size_t size, double precision = 1) {
    double float_size = static_cast<double>(size);
    const char* size_postfix;
    uint64_t power_of_size = 0;

    while (float_size >= 1024) {
        float_size /= 1024;
        power_of_size++;
    }
    switch (power_of_size) {
    case 0:     size_postfix = "B";        break;
    case 1:     size_postfix = "KiB";      break;
    case 2:     size_postfix = "MiB";      break;
    case 3:     size_postfix = "GiB";      break;
    case 4:     size_postfix = "TiB";      break;
    case 5:     size_postfix = "PiB";      break;
    case 6:     size_postfix = "EiB";      break;
    default:    size_postfix = "<ERROR>";  break;
    }
    return std::format("{} {}",
        std::round(float_size * precision) / precision,
        size_postfix
    );
}

std::pair<std::string_view, std::string_view> split_ext(std::string_view sv) {
    auto pos = sv.rfind('.');
    if (pos == std::string_view::npos) {
        return { sv, "" };
    }
    return { sv.substr(0, pos), sv.substr(pos) };
}

enum StreamMode {
    Unspecified = 0,
    Read,
    Write,
};

struct ByteStream {
    virtual ~ByteStream() {}
    virtual size_t read(std::span<char> bytes) = 0;
    virtual size_t write(std::span<const char> bytes) = 0;
    virtual void seek(size_t pos) = 0;
    virtual size_t get_position(void) = 0;
    virtual size_t get_size(void) = 0;
    virtual bool is_readable(void) = 0;
    virtual bool is_writable(void) = 0;
    virtual bool is_seekable(void) = 0;
    virtual bool flush(void) = 0;
};

struct FileByteStream : ByteStream {
    FileByteStream(const std::string& path, bool truncate) :
        FileByteStream(path.c_str(), truncate) {}
    FileByteStream(const char* path, bool truncate) :
        m_last_mode(StreamMode::Unspecified),
        m_fs(path, std::ios::out | std::ios::binary | (truncate ? 0 : std::ios::app)),
        m_position(0)
    {
        // Create if file does not exist
        if (!m_fs) {
            throw std::runtime_error("无法打开文件流");
        }
        m_fs.close();
        // Then open the file in read + write mode
        m_fs.open(path, std::ios::in | std::ios::out | std::ios::binary);
        if (!m_fs) {
            throw std::runtime_error("无法打开文件流");
        }
    }
    size_t read(std::span<char> bytes) override {
        trigger_mode_update(StreamMode::Read);
        m_fs.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        size_t count = m_fs.gcount();
        m_position += count;
        return count;
    }
    size_t write(std::span<const char> bytes) override {
        trigger_mode_update(StreamMode::Write);
        m_fs.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        size_t count = m_fs ? bytes.size() : 0;
        m_position += count;
        return count;
    }
    void seek(size_t pos) override {
        m_last_mode = StreamMode::Unspecified;
        m_fs.clear();
        m_fs.seekg(static_cast<std::streamoff>(pos), std::ios::beg);
        m_position = pos;
    }
    size_t get_position(void) override {
        return m_position;
    }
    size_t get_size(void) override {
        m_fs.clear();
        auto last_pos = m_fs.tellg();
        m_fs.seekg(0, std::ios::beg);
        m_fs.ignore(std::numeric_limits<std::streamsize>::max());
        size_t size = static_cast<size_t>(m_fs.gcount());
        m_fs.seekg(last_pos);
        return size;
    }
    bool is_readable(void) override {
        return true;
    }
    bool is_writable(void) override {
        return true;
    }
    bool is_seekable(void) override {
        return true;
    }

    bool flush(void) override {
        switch (m_last_mode) {
        case StreamMode::Read:
        case StreamMode::Write:
            m_fs.flush();
            break;
        default:
            break;
        }
        m_last_mode = StreamMode::Unspecified;
        return static_cast<bool>(m_fs);
    }

private:
    void trigger_mode_update(StreamMode new_mode) {
        if (m_last_mode == new_mode) { return; }
        flush();
        m_last_mode = new_mode;
    }

    std::fstream m_fs;
    StreamMode m_last_mode;
    size_t m_position;
};

struct BitStream {
    // NOTE: read_bits & write_bits: count indicates
    //       the maximum count of bits to read / write
    BitStream(std::unique_ptr<ByteStream> stm) : m_stm(std::move(stm)),
        m_cur_byte(0), m_cur_byte_loaded(false), m_bit_offset(0)
    {
        m_stm->seek(0);
    }
    ~BitStream() {
        if (m_stm) {
            flush();
        }
    }
    size_t read_bit(bool& v) {
        if (m_last_mode != StreamMode::Read) {
            flush();
            m_last_mode = StreamMode::Read;
        }
        if (!m_cur_byte_loaded) {
            char buf[1];
            if (m_stm->read(buf) != 1) { return 0; }
            m_cur_byte = buf[0];
            m_cur_byte_loaded = true;
        }
        v = static_cast<bool>(m_cur_byte & (1 << m_bit_offset));
        m_bit_offset++;
        if (m_bit_offset == CHAR_BIT) {
            // Turn around
            m_bit_offset = 0;
            m_cur_byte_loaded = false;
        }
        return 1;
    }
    size_t write_bit(bool b) {
        if (m_last_mode != StreamMode::Write) {
            flush();
            // Check if we need to read before writing
            if (!m_cur_byte_loaded && m_bit_offset != 0) {
                char buf[1];
                if (m_stm->read(buf) != 1) { return 0; }
                m_cur_byte = buf[0];
                m_cur_byte_loaded = true;
            }
            m_last_mode = StreamMode::Write;
        }
        const unsigned char mask = 1 << m_bit_offset;
        m_cur_byte = (m_cur_byte & ~mask) + (static_cast<unsigned char>(b) << m_bit_offset);
        m_bit_offset++;
        if (m_bit_offset == CHAR_BIT) {
            // Turn around
            char buf[1];
            buf[0] = m_cur_byte;
            auto pos = m_stm->get_position();
            if (m_cur_byte_loaded) { m_stm->seek(pos - 1); }
            if (m_stm->write(buf) != 1) {
                // Rollback
                if (m_cur_byte_loaded) { m_stm->seek(pos); }
                m_bit_offset--;
                return 0;
            }
            m_bit_offset = 0;
            m_cur_byte_loaded = false;
        }
        return 1;
    }
    size_t read_bits(uint64_t& v, size_t count) {
        // TODO: Optimize performance
        size_t read_cnt = 0;
        v = 0;
        while (read_cnt < std::min(size_t{ 64 }, count)) {
            bool b;
            if (read_bit(b) != 1) { break; }
            v += static_cast<uint64_t>(b) << read_cnt;
            read_cnt++;
        }
        return read_cnt;
    }
    size_t read_bits(std::vector<bool>& v, size_t count) {
        // TODO: Optimize performance
        size_t read_cnt = 0;
        v.clear();
        while (read_cnt < count) {
            bool b;
            if (read_bit(b) != 1) { break; }
            v.push_back(b);
            read_cnt++;
        }
        return read_cnt;
    }
    size_t write_bits(uint64_t v, size_t count) {
        // TODO: Optimize performance
        size_t wrote_cnt = 0;
        while (wrote_cnt < std::min(size_t{ 64 }, count)) {
            bool b = static_cast<bool>(v & (uint64_t{ 1 } << wrote_cnt));
            if (write_bit(b) != 1) { break; }
            wrote_cnt++;
        }
        return wrote_cnt;
    }
    size_t write_bits(const std::vector<bool>& v, size_t count) {
        // TODO: Optimize performance
        size_t wrote_cnt = 0;
        while (wrote_cnt < std::min(v.size(), count)) {
            bool b = v[wrote_cnt];
            if (write_bit(b) != 1) { break; }
            wrote_cnt++;
        }
        return wrote_cnt;
    }

    void seek(size_t pos) {
        const size_t major_off = pos / CHAR_BIT, minor_off = pos % CHAR_BIT;
        flush();
        if (major_off == m_stm->get_position() - m_cur_byte_loaded) {
            // Byte offsets equal
            m_bit_offset = static_cast<unsigned char>(minor_off);
        }
        else {
            // Byte offsets differ
            m_stm->seek(major_off);
            m_cur_byte_loaded = false;
            m_bit_offset = static_cast<unsigned char>(minor_off);
        }
    }
    bool flush(void) {
        switch (m_last_mode) {
        case StreamMode::Write:
            // Check if we have uncommitted bits
            if (m_bit_offset != 0) {
                char buf[1];
                if (!m_cur_byte_loaded) {
                    // Blend bits first
                    if (m_stm->read(buf) != 1) {
                        // Zero-pad non-existent bits, and assume we just read
                        // a byte with all bits set to 0
                        buf[0] = 0;
                        m_stm->seek(m_stm->get_position() + 1);
                    }
                    const unsigned char mask = (~0) << m_bit_offset;
                    m_cur_byte = (m_cur_byte & ~mask) + (buf[0] & mask);
                    m_cur_byte_loaded = true;
                }
                // Commit
                buf[0] = m_cur_byte;
                auto pos = m_stm->get_position();
                m_stm->seek(pos - 1);
                if (m_stm->write(buf) != 1) {
                    m_stm->seek(pos);
                    return false;
                }
            }
            m_stm->flush();
            break;
        default:
            break;
        }
        m_last_mode = StreamMode::Unspecified;
        return true;
    }

    // WARN: This method drains ownership of self
    //       (effectively moving resource out)
    std::unique_ptr<ByteStream> into_inner_stream(void) {
        return std::move(m_stm);
    }

private:
    std::unique_ptr<ByteStream> m_stm;
    unsigned char m_cur_byte;
    bool m_cur_byte_loaded;
    unsigned char m_bit_offset;
    StreamMode m_last_mode;
};

struct HuffmanTree {
    static HuffmanTree from_bytes(ByteStream& stm) {
        static_assert(CHAR_BIT == 8, "byte must be an octet");
        // NOTE: An additional code (<EOF>: 256) will be used to mark end
        //       of data when encoding / decoding
        struct compare {
            compare(const HuffmanTree& ht) : m_ht(ht) {}
            bool operator()(size_t a, size_t b) {
                // NOTE: Reverse comparison to construct a min-heap
                return m_ht.m_nodes[a].weight > m_ht.m_nodes[b].weight;
            }
        private:
            const HuffmanTree& m_ht;
        };
        HuffmanTree ht;
        ht.m_nodes.reserve(257 * 2 - 1);
        std::priority_queue<size_t, std::vector<size_t>, compare> heap(compare{ ht });
        size_t cnt_table[256]{};
        char buf[1];
        while (stm.read(buf) == 1) {
            cnt_table[static_cast<unsigned char>(buf[0])]++;
        }
        for (size_t i = 0; i < 256; i++) {
            ht.m_nodes.emplace_back(static_cast<unsigned>(i), cnt_table[i], i, i, i);
            heap.push(i);
        }
        ht.m_nodes.emplace_back(256, 1, 256, 256, 256);
        heap.push(256);
        const size_t aux_nodes_cnt = 257 - 1;
        for (size_t i = 0; i < aux_nodes_cnt; i++) {
            size_t min1_idx = heap.top(); heap.pop();
            size_t min2_idx = heap.top(); heap.pop();
            size_t new_node_idx = 257 + i;
            ht.m_nodes[min1_idx].parent = ht.m_nodes[min2_idx].parent = new_node_idx;
            ht.m_nodes.emplace_back(
                -1,
                ht.m_nodes[min1_idx].weight + ht.m_nodes[min2_idx].weight,
                new_node_idx,
                min1_idx, min2_idx
            );
            heap.push(new_node_idx);
        }
        return ht;
    }

    bool encode_bytes(ByteStream& in_stm, BitStream& out_stm) {
        // NOTE: Reversed huffman codes are written for ease of decoding
        std::vector<bool> codes[257];
        for (size_t i = 0; i < 257; i++) {
            codes[i] = gen_reversed_code_for_node(i);
        }
        auto write_fn = [&](unsigned index) {
            size_t cnt = codes[index].size();
            return out_stm.write_bits(codes[index], cnt) == cnt;
        };
        char buf[1];
        while (in_stm.read(buf) == 1) {
            if (!write_fn(static_cast<unsigned char>(buf[0]))) {
                return false;
            }
        }
        return write_fn(256);
    }

    bool decode_bytes(BitStream& in_stm, ByteStream& out_stm) {
        bool b;
        const size_t root_node_idx = m_nodes.size() - 1;
        size_t node_idx = root_node_idx;
        while (in_stm.read_bit(b) == 1) {
            node_idx = b ? m_nodes[node_idx].rchild : m_nodes[node_idx].lchild;
            if (node_idx >= 257) { continue; }
            if (node_idx == 256) {
                // End of data
                return true;
            }
            char buf[1];
            buf[0] = std::bit_cast<char>(static_cast<unsigned char>(node_idx));
            if (out_stm.write(buf) != 1) {
                return false;
            }
            node_idx = root_node_idx;
        }
        return false;
    }

    // NOTE: Returns [byte, frequency, huffman_code(index 0 is LSB)]
    std::vector<std::tuple<char, size_t, std::vector<bool>>> get_codes(void) {
        std::vector<std::tuple<char, size_t, std::vector<bool>>> result;
        for (size_t i = 0; i < 256; i++) {
            const auto& node = m_nodes[i];
            result.emplace_back(
                std::bit_cast<char>(static_cast<unsigned char>(node.data)),
                node.weight,
                gen_code_for_node(i)
            );
        }
        return result;
    }

private:
    HuffmanTree() {}

    size_t get_data_nodes_count(void) {
        return m_nodes.size() / 2 + 1;
    }
    // NOTE: Index 0 is LSB
    std::vector<bool> gen_code_for_node(size_t node_idx) const {
        std::vector<bool> code;
        while (m_nodes[node_idx].parent != node_idx) {
            size_t parent_idx = m_nodes[node_idx].parent;
            // Left: 0; right: 1
            code.push_back(m_nodes[parent_idx].rchild == node_idx);
            node_idx = parent_idx;
        }
        return code;
    }
    // NOTE: Index 0 is MSB
    std::vector<bool> gen_reversed_code_for_node(size_t node_idx) const {
        bool code_stack[256];
        size_t code_stack_depth = 0;
        std::vector<bool> code;
        while (m_nodes[node_idx].parent != node_idx) {
            size_t parent_idx = m_nodes[node_idx].parent;
            // Left: 0; right: 1
            code_stack[code_stack_depth++] = m_nodes[parent_idx].rchild == node_idx;
            node_idx = parent_idx;
        }
        while (code_stack_depth > 0) {
            code_stack_depth--;
            code.push_back(code_stack[code_stack_depth]);
        }
        return code;
    }

    struct Node {
        unsigned data;
        size_t weight;
        size_t parent;  // parent == self_index: no parent
        size_t lchild, rchild;
    };
    std::vector<Node> m_nodes;
};

int main(void) try {
    std::string input;
    while (true) {
        fmt_println("输入欲处理的文件路径: ");
        if (!getline(std::cin, input)) {
            break;
        }
        // auto [filename, filext] = split_ext(input);
        const std::string stat_path = "Huffman.txt";
        const std::string encode_path = "code.dat";
        const std::string recode_path = "recode.txt";
        try {
            std::unique_ptr<ByteStream> src_fs = std::make_unique<FileByteStream>(input, false);
            auto ht = HuffmanTree::from_bytes(*src_fs);
            {
                fmt_println("正在输出统计信息到 `{}` ...", stat_path);
                std::ofstream stat_fs(stat_path);
                fmt_writeln(stat_fs, "字符\t出现次数\t编码");
                for (auto [byte, freq, code] : ht.get_codes()) {
                    std::string code_str;
                    for (bool i : code | std::ranges::views::reverse) {
                        code_str += i ? '1' : '0';
                    }
                    if (std::isgraph(byte)) {
                        fmt_writeln(stat_fs, "{}\t{}\t{}", byte, freq, code_str);
                    }
                    else {
                        fmt_writeln(stat_fs, "0x{:02x}\t{}\t{}",
                            static_cast<unsigned char>(byte), freq, code_str);
                    }
                }
            }
            fmt_println("正在输出编码后文件到 `{}` ...", encode_path);
            src_fs->seek(0);
            BitStream encode_fs{ std::make_unique<FileByteStream>(encode_path, true) };
            ht.encode_bytes(*src_fs, encode_fs);
            fmt_println("正在输出解码后文件到 `{}` ...", recode_path);
            encode_fs.seek(0);
            std::unique_ptr<ByteStream> recode_fs = std::make_unique<FileByteStream>(recode_path, true);
            ht.decode_bytes(encode_fs, *recode_fs);
            auto orig_encode_fs = encode_fs.into_inner_stream();
            fmt_println("* 大小变化: {} -> {}",
                byte_size_to_str(src_fs->get_size(), 1e2),
                byte_size_to_str(orig_encode_fs->get_size(), 1e2)
            );
        }
        catch (const std::exception& e) {
            fmt_println("错误: {}", e.what());
        }
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
