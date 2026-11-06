#pragma once

#include <bits/stdc++.h>

using namespace std;

struct Line {
    bool valid_{false};
    uint32_t time_{0};
    uint64_t tag_{0};
    Line() = default;
    Line(bool valid, uint32_t time, uint64_t tag) : 
            valid_(valid), time_(time), tag_(tag) {}
};

struct Set {
    size_t line_counts{0};
    vector<Line> lines_;
    Set() = default;
    Set(size_t e, uint32_t time){
        line_counts = e;
        lines_.reserve(e);
        for (size_t i = 0; i < line_counts; ++i)
            lines_.push_back(Line(false, time, 0));
    }
    bool isFull() {
        for (const auto &line : lines_)
            if (!line.valid_)
                return false;
        return true;
    }
    Line& evict() {
        return *min_element(lines_.begin(), lines_.end(), [](const Line &a, const Line &b) { return a.time_ < b.time_; });
    }
};

class Cache {
private:
    std::size_t set_{0};
    std::size_t lines_{0};
    std::size_t bits_{0};
    bool verbose_{false};

    std::size_t hits{0};
    std::size_t misses{0};
    std::size_t evictions{0};

    std::atomic<uint32_t> counter{0};

private:
    vector<Set> sets_{};

public:
    explicit Cache(size_t set, size_t lines, size_t bits, bool verbose = false) :
                    set_{set}, lines_{lines}, bits_{bits}, verbose_{verbose} {
        size_t set_counts = 1 << set_;
        sets_.reserve(set_counts);
        uint32_t time = counter.load(memory_order_relaxed);
        for (size_t i = 0; i < set_counts; ++i)
            sets_.push_back(Set(lines_, time));
    }

    void showResults() {
        printf("hits:%zu misses:%zu evictions:%zu\n", hits, misses, evictions);
        FILE* output_fp = fopen(".csim_results", "w");
        assert(output_fp);
        fprintf(output_fp, "%zu %zu %zu\n", hits, misses, evictions);
        fclose(output_fp);
    }

    std::size_t hitCount() const noexcept { return hits; }
    std::size_t missCount() const noexcept { return misses; }
    std::size_t evictionCount() const noexcept { return evictions; }

public:
    void Access(uint64_t address, size_t bytes) noexcept {
        uint32_t cur_time = counter.fetch_add(1, std::memory_order_relaxed) + 1;
        uint64_t set_index = (address >> bits_) & ((1 << set_) - 1);
        uint64_t tag = address >> (set_ + bits_);

        Set& cur_set = sets_[set_index];
        for (auto &line : cur_set.lines_) {
            if (line.valid_ && tag == line.tag_) {
                hits++;
                line.time_ = cur_time;
                if (verbose_)
                    cout << "hit ";
                return;   
            }   
        }
        misses++;
        if (verbose_)
            cout << "miss ";
        if (!cur_set.isFull()) {
            for (auto &line : cur_set.lines_) {
                if (!line.valid_) {
                    line.valid_ = true;
                    line.tag_ = tag;
                    line.time_ = cur_time;
                    return;
                }
            }
        } else {
            if (verbose_)
                cout << "eviction ";
            evictions++;
            Line& eviction = cur_set.evict();
            eviction.tag_ = tag;
            eviction.time_ = cur_time;
            eviction.valid_ = true;
        }
    }

    void Load(uint64_t address, size_t bytes) noexcept {
        if (verbose_)
            cout << "L " << std::hex << address << std::dec << "," << bytes << " ";
        Access(address, bytes);
        if (verbose_)
            cout << endl;
    }

    void Store(uint64_t address, size_t bytes) noexcept {
        if (verbose_)
            cout << "S " << std::hex << address << std::dec << "," << bytes << " ";
        Access(address, bytes);
        if (verbose_)
            cout << endl;
    }

    void Modify(uint64_t address, size_t bytes) noexcept {
        if (verbose_)
            cout << "M " << std::hex << address << std::dec << "," << bytes << " ";
        Access(address, bytes);
        Access(address, bytes);
        if (verbose_)
            cout << endl;
    }
};
