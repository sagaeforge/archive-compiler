#include "kern/support/BuildCache.h"
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace kern {

BuildCache::BuildCache(const std::string& cache_dir)
    : cache_dir_(cache_dir) {}

bool BuildCache::ensureCacheDir() {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(cache_dir_, ec);
    return !ec;
}

uint64_t BuildCache::fnv1a(const std::string& data) {
    uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : data) {
        hash ^= c;
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::string BuildCache::cacheKey(const std::string& source_content,
                                  const std::string& flags) const {
    std::string combined = source_content + "\0" + flags;
    uint64_t hash = fnv1a(combined);
    std::ostringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}

std::string BuildCache::cachedPath(const std::string& key) const {
    return cache_dir_ + "/" + key + ".o";
}

std::string BuildCache::lookup(const std::string& key) const {
    std::string path = cachedPath(key);
    if (std::filesystem::exists(path)) {
        ++hits_;
        return path;
    }
    ++misses_;
    return "";
}

bool BuildCache::store(const std::string& key, const std::string& obj_file) {
    if (!ensureCacheDir()) return false;
    std::string dest = cachedPath(key);
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::copy_file(obj_file, dest, fs::copy_options::overwrite_existing, ec);
    return !ec;
}

} // namespace kern
