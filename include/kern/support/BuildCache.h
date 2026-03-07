#pragma once
#include <cstdint>
#include <string>

namespace kern {

// Simple incremental build cache for separate compilation.
// Stores .o files keyed by source content hash. When source content
// and compiler flags haven't changed, reuses the cached .o file.
class BuildCache {
public:
    // cache_dir: directory for cached .o files (e.g., ".kern-cache")
    explicit BuildCache(const std::string& cache_dir);

    // Compute a cache key for a source file + compiler options.
    // Key = fnv1a(source_content + flags_string)
    std::string cacheKey(const std::string& source_content,
                         const std::string& flags) const;

    // Check if a valid cached .o exists for the given key.
    // Returns the path if it exists and is valid, empty string otherwise.
    std::string lookup(const std::string& key) const;

    // Store a compiled .o in the cache under the given key.
    // Returns true on success.
    bool store(const std::string& key, const std::string& obj_file);

    // Ensure the cache directory exists.
    bool ensureCacheDir();

    // Get cache stats
    uint32_t hits() const { return hits_; }
    uint32_t misses() const { return misses_; }

private:
    std::string cache_dir_;
    mutable uint32_t hits_ = 0;
    mutable uint32_t misses_ = 0;

    static uint64_t fnv1a(const std::string& data);
    std::string cachedPath(const std::string& key) const;
};

} // namespace kern
