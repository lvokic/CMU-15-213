// cachelab.c is compiled as C; use C linkage for its summary function.
extern "C" void printSummary(int hits, int misses, int evictions);

#include "cache.hpp"
#include <charconv>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unistd.h>

namespace {

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n";
}

bool parseNumber(const char* text, std::size_t& value) {
    std::string_view input(text);
    const auto result = std::from_chars(input.data(), input.data() + input.size(), value);
    return !input.empty() && result.ec == std::errc{} && result.ptr == input.data() + input.size();
}

} // namespace

int main(int argc, char* argv[]) {
    std::size_t s = 0;
    std::size_t E = 0;
    std::size_t b = 0;
    std::string trace_file;
    bool verbose = false;
    bool has_s = false;
    bool has_E = false;
    bool has_b = false;

    opterr = 0;
    int option;
    while ((option = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (option) {
            case 'h':
                printUsage(argv[0]);
                return 0;
            case 'v':
                verbose = true;
                break;
            case 's':
                has_s = parseNumber(optarg, s);
                if (!has_s) {
                    std::cerr << "Invalid value for -s: " << optarg << '\n';
                    return 1;
                }
                break;
            case 'E':
                has_E = parseNumber(optarg, E);
                if (!has_E) {
                    std::cerr << "Invalid value for -E: " << optarg << '\n';
                    return 1;
                }
                break;
            case 'b':
                has_b = parseNumber(optarg, b);
                if (!has_b) {
                    std::cerr << "Invalid value for -b: " << optarg << '\n';
                    return 1;
                }
                break;
            case 't':
                trace_file = optarg;
                break;
            default:
                std::cerr << "Unknown option or missing option value.\n";
                printUsage(argv[0]);
                return 1;
        }
    }

    if (!has_s || !has_E || !has_b || trace_file.empty() || optind != argc) {
        std::cerr << "Missing required option or unexpected argument.\n";
        printUsage(argv[0]);
        return 1;
    }
    // Cache currently uses 1 << s with an int literal when creating sets.
    if (E == 0 || s >= std::numeric_limits<int>::digits || b >= 64 || s + b >= 64) {
        std::cerr << "Invalid cache configuration: E must be positive, s < "
                  << std::numeric_limits<int>::digits << ", and s + b < 64.\n";
        return 1;
    }

    std::ifstream trace(trace_file);
    if (!trace) {
        std::cerr << "Cannot open trace file: " << trace_file << '\n';
        return 1;
    }

    Cache cache(s, E, b, verbose);
    std::string line;
    while (std::getline(trace, line)) {
        std::istringstream input(line);
        char operation;
        char comma;
        std::uint64_t address;
        std::size_t bytes;

        if (!(input >> operation) || operation == 'I') {
            continue;
        }
        if (!(input >> std::hex >> address >> comma >> std::dec >> bytes) || comma != ',') {
            std::cerr << "Invalid trace line: " << line << '\n';
            return 1;
        }

        switch (operation) {
            case 'L':
                cache.Load(address, bytes);
                break;
            case 'S':
                cache.Store(address, bytes);
                break;
            case 'M':
                cache.Modify(address, bytes);
                break;
            default:
                std::cerr << "Unknown trace operation: " << operation << '\n';
                return 1;
        }
    }
    if (trace.bad()) {
        std::cerr << "Error reading trace file: " << trace_file << '\n';
        return 1;
    }

    const auto max_summary_count = static_cast<std::size_t>(std::numeric_limits<int>::max());
    if (cache.hitCount() > max_summary_count || cache.missCount() > max_summary_count ||
        cache.evictionCount() > max_summary_count) {
        std::cerr << "Trace counts exceed the range supported by printSummary.\n";
        return 1;
    }
    printSummary(static_cast<int>(cache.hitCount()), static_cast<int>(cache.missCount()),
                 static_cast<int>(cache.evictionCount()));
    return 0;
}
