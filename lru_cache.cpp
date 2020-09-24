#include <cstdint>
#include "lru_cache.h"
#include "buffer.h"

namespace logfind 
{
    LRUCache::LRUCache(uint8_t maxsize)
    :maxsize_(maxsize)
    {

    }

    LRUCache::~LRUCache()
    {
        clear();
    }

    void LRUCache::clear()
    {
        for (auto& b : list_)
        {
            delete b;
        }
        list_.clear();
        map_.clear();
    }

    Buffer *LRUCache::get_lru()
    {
        if (map_.size() < maxsize_)
            return new Buffer();

        return remove_lru_();
    }

    // Get a buffer. Buffer will become the most recently used
    Buffer *LRUCache::get(uint32_t key)
    {
        Buffer *pBuf = remove_key_(key);
        if (pBuf)
            add(pBuf);
        return pBuf;
    }

    void LRUCache::add(Buffer *pBuf)
    {
        if (maxsize_ == map_.size())
        {
            Buffer *p = remove_lru_();
            delete p;
        }
        add_(pBuf);
    }

    Buffer * LRUCache::remove_lru_()
    {
        if (map_.size() != maxsize_)
            return nullptr;
        Buffer *pBuf = list_.back();
        list_.pop_back();
        map_.erase(pBuf->fileoffset());
        return pBuf;
    }

    Buffer * LRUCache::remove_key_(uint64_t k)
    {
        auto map_iter = map_.find(k);
        if (map_iter != map_.end())
        {
            auto list_iter = map_iter->second;
            Buffer *pBuf = *list_iter;
            list_.erase(list_iter);
            map_.erase(map_iter);
            return pBuf;
        }
        return nullptr;
    }

    void LRUCache::add_(Buffer *pBuf)
    {
        if (map_.count(pBuf->fileoffset()))
            return;
        list_.push_front(pBuf);
        map_[pBuf->fileoffset()] = list_.begin();
    }
} 

