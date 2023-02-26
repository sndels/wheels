#ifndef WHEELS_SCOPED_SCRATCH_HPP
#define WHEELS_SCOPED_SCRATCH_HPP

#include "linear_allocator.hpp"
#include "utils.hpp"

#include <cassert>
#include <climits>
#include <cstdint>

// Implements Frostbite's Scope Stack:
// https://www.ea.com/frostbite/news/scope-stack-allocation

namespace wheels
{

struct ScopeData
{
    // The Frostbite slides infer the data pointer from the scope address, but
    // let's just have the extra 8 bytes for now as it's should be safe
    // everywhere.
    void *data{nullptr};
    void (*dtor)(void *ptr);
    ScopeData *previous{nullptr};
};

class ScopedScratch
{
  public:
    ScopedScratch(LinearAllocator &allocator)
    : m_allocator{allocator}
    , m_alloc_start{allocator.peek()}
    {
    }

    virtual ~ScopedScratch()
    {
        if (m_alloc_start != nullptr)
        {
            ScopeData *scope = m_objects;
            while (scope != nullptr)
            {
                scope->dtor(scope->data);
                scope = scope->previous;
            }

            m_allocator.rewind(m_alloc_start);

            if (m_parent_scope)
                m_parent_scope->m_has_child_scope = false;
        }
    };

    ScopedScratch(ScopedScratch const &) = delete;
    ScopedScratch(ScopedScratch &&other)
    : m_allocator{other.m_allocator}
    , m_alloc_start{other.m_alloc_start}
    , m_parent_scope{other.m_parent_scope}
    , m_has_child_scope{other.m_has_child_scope}
    , m_objects{other.m_objects}
    {
        other.m_alloc_start = nullptr;
    };

    ScopedScratch &operator=(ScopedScratch const &) = delete;
    ScopedScratch &operator=(ScopedScratch &&) = delete;

    ScopedScratch child_scope()
    {
        assert(
            !m_has_child_scope && "Tried to create a child scope from a "
                                  "ScopedScratch that already has one");

        ScopedScratch scope{m_allocator};
        scope.m_parent_scope = this;

        m_has_child_scope = true;

        return scope;
    }

    template <typename T> T *allocate_pod()
    {
        assert(
            !m_has_child_scope &&
            "Tried to allocate from a ScopedScratch that has a child scope");

        return (T *)m_allocator.allocate(sizeof(T));
    }

    template <typename T> T *allocate_object()
    {
        assert(
            !m_has_child_scope &&
            "Tried to allocate from a ScopedScratch that has a child scope");

        ScopeData *scope = (ScopeData *)m_allocator.allocate(sizeof(ScopeData));
        if (scope == nullptr)
            return nullptr;

        scope->data = m_allocator.allocate(sizeof(T));
        if (scope->data == nullptr)
        {
            m_allocator.rewind(scope);
            return nullptr;
        }

        scope->dtor = &dtor_call<T>;
        scope->previous = m_objects;

        m_objects = scope;

        return (T *)scope->data;
    }

  private:
    template <typename T> static void dtor_call(void *ptr) { ((T *)ptr)->~T(); }

    LinearAllocator &m_allocator;
    void *m_alloc_start{nullptr};
    ScopedScratch *m_parent_scope{nullptr};
    bool m_has_child_scope{false};

    ScopeData *m_objects{nullptr};
};

} // namespace wheels

#endif // WHEELS_SCOPED_SCRATCH_HPP