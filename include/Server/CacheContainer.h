#ifndef DASHSRV_CACHECONTAINER_H__
#define DASHSRV_CACHECONTAINER_H__

#include <cstdint>

#include <Basic.h>

template<typename Store, uint64_t CacheTimerMS>
class CacheContainer {
 public:
    CacheContainer() {
        lastFetchedTimeMS = 0;
    }
    
    bool NeedsFetch() {
        return GetTimeMillis() - lastFetchedTimeMS > CacheTimerMS;
    }

    const Store &Cache(const Store &storing) {
        stored = storing;
        lastFetchedTimeMS = GetTimeMillis();
        return stored;
    }

    const Store &Get() {
        return stored;
    }

    uint64_t GetTiming() {
        return lastFetchedTimeMS;
    }

  private:
    uint64_t lastFetchedTimeMS;
    Store stored;
};

#endif // DASHSRV_CACHECONTAINER_H__
