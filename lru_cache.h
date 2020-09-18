#ifndef LRUCACHE_H_
#define LRUCACHE_H_

#include <unordered_map>
#include <list>
#include <cstddef>
#include <stdexcept>

namespace logfind 
{
    class Buffer;

    class LRUCache
    {
    public:
        LRUCache(uint8_t maxsize);
        ~LRUCache();

        // Get a buffer for reuse. Must call add() to put back in cache
        Buffer *get_lru();

        // Get a buffer. Buffer will become the most recently used
        Buffer *get(uint32_t key);

        // Add a buffer to the cache
        void add(Buffer *pBuf);

        void clear();
    private:
        // Get the LRU buffer and remove from the cache
        Buffer * remove_lru_();

        // Get a buffer and remove from the cache
        Buffer * remove_key_(uint64_t k);

        void add_(Buffer *pBuf);

        using list_t = std::list<Buffer *>;
        list_t list_;
        std::unordered_map<uint64_t, list_t::iterator> map_;
        int maxsize_;
    };
} 

#endif 
