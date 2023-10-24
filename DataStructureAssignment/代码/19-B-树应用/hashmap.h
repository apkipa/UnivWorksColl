#pragma once

#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

namespace hashmap {
    template<typename T>
    struct lite_hash;

    template<>
    struct lite_hash<std::string_view> {
        size_t operator()(std::string_view v) const {
            const size_t p = 257, m = static_cast<size_t>(1e9 + 9);
            size_t hash_value = 0;
            size_t p_pow = 1;
            for (char ch : v) {
                hash_value = (hash_value + ch * p_pow) % m;
                p_pow = (p_pow * p) % m;
            }
            return hash_value;
        }
    };
    template<>
    struct lite_hash<std::string> {
        size_t operator()(const std::string& v) const {
            return lite_hash<std::string_view>{}(v);
        }
    };
    template<>
    struct lite_hash<int> {
        size_t operator()(const int& v) const {
            return static_cast<size_t>(v);
        }
    };

    // NOTE: Heterogeneous lookup not implemented for now
    template<typename Key, typename T, typename Hash = lite_hash<Key>,
        typename KeyEqual = std::equal_to<Key>>
    struct HashMap {
        using key_type = Key;
        using mapped_type = T;
        using Container = std::vector<std::unique_ptr<std::pair<const Key, T>>>;

        using value_type = std::pair<const Key, T>;
        using allocator_type = typename Container::allocator_type;
        using hasher = Hash;
        using size_type = typename Container::size_type;
        using difference_type = typename Container::difference_type;
        using pointer = value_type*;
        using const_pointer = const value_type*;
        using reference = value_type&;
        using const_reference = const value_type&;

        struct const_iterator {
            const_iterator(const Container* pb, const Container* pe, typename Container::const_iterator it) :
                bucket_ptr(pb), bucket_end(pe), inner_it(it) {}
            // Precondition: self != end()
            const_iterator& operator++() {
                ++inner_it;
                while (inner_it == bucket_ptr->end()) {
                    bucket_ptr++;
                    if (bucket_ptr == bucket_end) { break; }
                    inner_it = bucket_ptr->begin();
                }
                return *this;
            }
            const_iterator operator++(int) {
                auto copy = *this;
                operator++();
                return copy;
            }
            // Precondition: self != begin()
            const_iterator& operator--() {
                while (bucket_ptr == bucket_end || inner_it == bucket_ptr->begin()) {
                    bucket_ptr--;
                    inner_it = bucket_ptr->end();
                }
                --inner_it;
                return *this;
            }
            const_iterator operator--(int) {
                auto copy = *this;
                operator--();
                return copy;
            }
            const_reference operator*() {
                return *inner_it.operator*();
            }
            const_pointer operator->() {
                return &**inner_it.operator->();
            }
            bool operator==(const_iterator rhs) {
                if (bucket_ptr == bucket_end && rhs.bucket_ptr == bucket_end) {
                    return true;
                }
                if (bucket_ptr != rhs.bucket_ptr) { return false; }
                return inner_it == rhs.inner_it;
            }
        private:
            friend struct HashMap;

            const Container* bucket_ptr;
            const Container* bucket_end;
            typename Container::const_iterator inner_it;
        };
        struct iterator : const_iterator {
            using const_iterator::const_iterator;
            iterator(const const_iterator& other) : const_iterator(other) {}
            iterator& operator++() {
                const_iterator::operator++();
                return *this;
            }
            iterator operator++(int) {
                auto copy = *this;
                const_iterator::operator++();
                return copy;
            }
            iterator& operator--() {
                const_iterator::operator--();
                return *this;
            }
            iterator operator--(int) {
                auto copy = *this;
                const_iterator::operator--();
                return copy;
            }
            reference operator*() {
                return const_cast<reference>(const_iterator::operator*());
            }
            pointer operator->() {
                return const_cast<pointer>(const_iterator::operator->());
            }
        private:
            friend struct HashMap;
        };

        HashMap() : equal(KeyEqual{}), hash(Hash{}), buckets{} {}
        HashMap(const HashMap& other) : HashMap() {
            for (size_t i = 0; i < BUCKETS_COUNT; i++) {
                const auto& other_bucket = other.buckets[i];
                auto other_size = other_bucket.size();
                buckets[i].reserve(other_size);
                for (size_t j = 0; j < other_size; j++) {
                    buckets[i].push_back(std::make_unique<value_type>(*other_bucket[j]));
                }
            }
        }
        ~HashMap() = default;
        HashMap& operator=(HashMap rhs) noexcept { swap(*this, rhs); return *this; }

        const_iterator cbegin() const noexcept {
            const Container* bucket_ptr = buckets;
            const Container* bucket_end = buckets + BUCKETS_COUNT;
            typename Container::const_iterator it = bucket_ptr->begin();
            while (it == bucket_ptr->end()) {
                bucket_ptr++;
                if (bucket_ptr == bucket_end) { break; }
                it = bucket_ptr->begin();
            }
            return { bucket_ptr, bucket_end, it };
        }
        iterator begin() noexcept { return this->cbegin(); }
        const_iterator begin() const noexcept { return this->cbegin(); }
        const_iterator cend() const noexcept {
            const Container* bucket_end = buckets + BUCKETS_COUNT;
            return { bucket_end, bucket_end, buckets[0].begin() };
        }
        iterator end() noexcept { return this->cend(); }
        const_iterator end() const noexcept { return this->cend(); }

        const mapped_type& at(const Key& key) const {
            for (const auto& i : get_bucket(hash(key))) {
                if (equal(key, i->first)) {
                    return i->second;
                }
            }
            throw std::out_of_range("Key not found in HashMap");
        }
        mapped_type& at(const Key& key) {
            return const_cast<mapped_type&>(std::as_const(*this).at(key));
        }
        bool empty() const noexcept {
            for (const auto& bucket : buckets) {
                if (!bucket.empty()) {
                    return false;
                }
            }
            return true;
        }
        size_type size() const noexcept {
            size_type s = 0;
            for (const auto& bucket : buckets) {
                s += bucket.size();
            }
            return s;
        }
        size_type max_size() const noexcept {
            return buckets[0].max_size();
        }
        void clear() noexcept {
            for (auto& bucket : buckets) {
                bucket.clear();
            }
        }
        iterator erase(iterator pos) noexcept {
            iterator result_it = pos;
            if (pos.inner_it + 1 == pos.bucket_ptr->end()) {
                // Move iterator across buckets
                auto old_inner_it = result_it.inner_it;
                ++result_it;
                (const_cast<Container*>(pos.bucket_ptr))->erase(old_inner_it);
            }
            else {
                // Move iterator within one bucket
                result_it.inner_it = (const_cast<Container*>(pos.bucket_ptr))->erase(result_it.inner_it);
            }
            return result_it;
        }
        const_iterator find(const Key& key) const {
            const auto& bucket = get_bucket(hash(key));
            auto local_it = bucket.begin();
            auto local_ie = bucket.end();
            while (local_it != local_ie) {
                if (equal(key, (*local_it)->first)) {
                    return { &bucket, buckets + BUCKETS_COUNT, local_it };
                }
                ++local_it;
            }
            return this->cend();
        }
        iterator find(const Key& key) {
            return std::as_const(*this).find(key);
        }
        size_type count(const Key& key) const {
            return find(key) != this->end();
        }
        bool contains(const Key& key) const { return this->count(key) > 0; }
        std::pair<iterator, bool> insert(const value_type& value) {
            auto& bucket = get_bucket(hash(value.first));
            auto local_it = bucket.begin();
            auto local_ie = bucket.end();
            while (local_it != local_ie) {
                if (equal(value.first, local_it->first)) {
                    return std::pair{ iterator{ &bucket, buckets + BUCKETS_COUNT, local_it }, false };
                }
                ++local_it;
            }
            bucket.push_back(std::move(value));
            local_it = bucket.end() - 1;
            return std::pair{ iterator{ &bucket, buckets + BUCKETS_COUNT, local_it }, true };
        }
        std::pair<iterator, bool> insert(value_type&& value) {
            auto& bucket = get_bucket(hash(value.first));
            auto local_it = bucket.begin();
            auto local_ie = bucket.end();
            while (local_it != local_ie) {
                if (equal(value.first, (*local_it)->first)) {
                    return std::pair{ iterator{ &bucket, buckets + BUCKETS_COUNT, local_it }, false };
                }
                ++local_it;
            }
            bucket.push_back(std::make_unique<value_type>(std::move(value)));
            local_it = bucket.end() - 1;
            return std::pair{ iterator{ &bucket, buckets + BUCKETS_COUNT, local_it }, true };
        }

        bool operator==(const HashMap& rhs) const {
            for (size_t i = 0; i < BUCKETS_COUNT; i++) {
                const Container& ca = buckets[i];
                const Container& cb = rhs.buckets[i];
                if (ca.size() != cb.size()) {
                    return false;
                }
                for (size_t j = 0; j < ca.size(); j++) {
                    bool ok = false;
                    for (size_t k = 0; k < cb.size(); k++) {
                        if (*ca[j] == *cb[k]) {
                            ok = true;
                            break;
                        }
                    }
                    if (!ok) {
                        return false;
                    }
                }
            }
            return true;
        }
        T& operator[](const Key& key) {
            auto iter = this->find(key);
            if (iter != this->end()) {
                return iter->second;
            }
            else {
                return this->insert(value_type{ key, {} }).first->second;
            }
        }

        friend void swap(HashMap& lhs, HashMap& rhs) noexcept {
            using std::swap;
            swap(lhs.equal, rhs.equal);
            swap(lhs.hash, rhs.hash);
            swap(lhs.buckets, rhs.buckets);
        }

    private:
        static constexpr size_t BUCKETS_COUNT = 64;

        const Container& get_bucket(size_t hash_value) const {
            // *Naive* solution
            return buckets[hash_value & (BUCKETS_COUNT - 1)];
        }
        Container& get_bucket(size_t hash_value) {
            return const_cast<Container&>(std::as_const(*this).get_bucket(hash_value));
        }

        KeyEqual equal;
        Hash hash;
        Container buckets[BUCKETS_COUNT];
    };
}
