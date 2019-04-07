#pragma once

#include <cstdint>
#include <algorithm>
#include <cassert>
#include <cstring>

struct Handle {
    uint32_t index : 24;
    uint32_t generation : 8;

    inline bool operator==(Handle handle) const {
        return *(const uint32_t*)this == *(const uint32_t*)&handle;
    }

    inline bool operator!=(Handle handle) const {
        return *(const uint32_t*)this != *(const uint32_t*)&handle;
    }
};

static const Handle InvalidHandle = {0xFFFFFF, 0xFF};

template <const size_t MaxHandles>
class HandleManager {
public:
    struct Element {
        uint32_t index : 24;
        uint32_t generation : 8;
        uint32_t prev;
        uint32_t next;
    } ;

    HandleManager()
    {
        memset(elements, 0, sizeof(Element) * MaxHandles);
        elements[MaxHandles - 1].index = MaxHandles;
        freeHead = 1;

        for (size_t i = freeHead; i < MaxHandles;) {
            elements[i].index = (uint32_t)i;
            elements[i].prev = i - 1;
            elements[i].next = i + 1;
            i = elements[i].next;
        }
        
        elements[MaxHandles - 1].next = 0;
    }

    Handle Allocate() {
        Handle handle = {};

        handle.index = elements[freeHead].index;
        handle.generation = elements[freeHead].generation;

        uint32_t currentHead = freeHead;
        auto& e = elements[currentHead];

        assert(freeHead != 0);
        assert(e.prev == 0);

        elements[e.next].prev = e.prev;
        freeHead = e.next;
        e.prev = 0;
        e.next = usedHead;
        usedHead = currentHead;

        return handle;
    }

    bool IsValid(Handle handle) const {
        return (handle.index != InvalidHandle.index) && (handle.generation == elements[handle.index].generation);
    }

    void Release(Handle handle) {
        if (!IsValid(handle))
            return;
       auto& e = elements[handle.index];
       elements[e.prev].next = e.next;
       elements[e.next].prev = e.prev;
       e.next = freeHead;
       freeHead = handle.index;
       e.generation++;
    }

private:
    Element elements[MaxHandles];
    uint32_t top;
    uint32_t freeHead;
    uint32_t usedHead;
};

template <typename T, const size_t MaxElements>
struct ResourceManager {
    T data[MaxElements];
    HandleManager<MaxElements> handles;
    using OnRelease = void (*) (ResourceManager* resourceManager, Handle handle);

    OnRelease releaseCallback;

    ResourceManager() :
        releaseCallback(nullptr)
    {
    }

    inline Handle Allocate() {
        return handles.Allocate();
    }

    inline bool IsValid(Handle handle) const {
        return handles.IsValid(handle);
    }

    T* operator[](Handle handle) {
        T* resource = nullptr;
        if (IsValid(handle))
            resource = &data[handle.index];
        return resource;
    }

    const T* operator[](Handle handle) const {
        const T* resource = nullptr;
        if (IsValid(handle))
            resource = &data[handle.index];
        return resource;
    }

    inline void Release(Handle handle) {
        if (releaseCallback)
            releaseCallback(this, handle);
        handles.Release(handle);
    }
};