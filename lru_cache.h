#ifndef LRUCACHE_H_
#define LRUCACHE_H_

#include <unordered_map>
#include <list>
#include <cstddef>
#include <stdexcept>

namespace logfind {

    class LRUCache
    {
    public:
        LRUCache(uint8_t maxsize)
        :maxsize_(maxsize)
        {

        }

        ~LRUCache()
        {
            for (auto& b : list_)
            {
                delete b;
            }
        }
        // Get a buffer for reuse. Must call add() to put back in cache
        Buffer *get_lru()
        {
            if (map_.size() < maxsize_)
                return new Buffer();

            return remove_lru_();
        }

        // Get a buffer. Buffer will become the most recently used
        Buffer *get(uint32_t key)
        {
            Buffer *pBuf = remove_key_(key);
            add(pBuf);
            return pBuf;
        }

        // Add a buffer to the cache
        void add(Buffer *pBuf)
        {
            if (maxsize_ == map_.size())
            {
                Buffer *pBuf = remove_lru_();
                delete pBuf;
            }
            add_(pBuf);
        }

    private:
        // Get the LRU buffer and remove from the cache
        Buffer * remove_lru_()
        {
            if (map_.size() != maxsize_)
                return nullptr;
            Buffer *pBuf = list_.back();
            list_.pop_back();
            map_.erase(pBuf->fileoffset());
            return pBuf;
        }

        // Get a buffer and remove from the cache
        Buffer * remove_key_(uint64_t k)
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

        void add_(Buffer *pBuf)
        {
            if (map_.count(pBuf->fileoffset()))
                return;
            list_.push_front(pBuf);
            map_[pBuf->fileoffset()] = list_.begin();
        }

        using list_t = std::list<Buffer *>;
        list_t list_;
        std::unordered_map<uint64_t, list_t::iterator> map_;
        int maxsize_;
    };
} 

#endif 
