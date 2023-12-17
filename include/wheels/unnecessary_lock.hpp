#ifndef WHEELS_UNNECESSARY_LOCK
#define WHEELS_UNNECESSARY_LOCK

#include "assert.hpp"

// From Game Engine Architecture 3rd ed. (4.9.7.5)
// By Gregory

// This can detect races in places that are thread unsafe by design.

#ifdef DISABLE_WHEELS_ASSERT
#define WHEELS_ASSERT_LOCK_NOT_NECESSARY(lock)
#else // !DISABLE_WHEELS_ASSERT
#define WHEELS_ASSERT_LOCK_NOT_NECESSARY(lock)                                 \
    const UnnecessaryLockJanitor _assert_lock(lock)
#endif // DISABLE_WHEELS_ASSERT

class UnnecessaryLock
{
  public:
    UnnecessaryLock() = default;

    void acquire()
    {
        WHEELS_ASSERT(
            !m_locked && "Non-thread safe code called from multiple threads");

        m_locked = true;
    }

    void release()
    {
        WHEELS_ASSERT(m_locked && "Acquire not called before release");

        m_locked = false;
    }

  private:
    volatile bool m_locked{false};
};

class UnnecessaryLockJanitor
{
  public:
    UnnecessaryLockJanitor(UnnecessaryLock &lock)
    : m_lock{lock}
    {
        m_lock.acquire();
    }

    ~UnnecessaryLockJanitor() { m_lock.release(); }

  private:
    UnnecessaryLock &m_lock;
};

#endif // WHEELS_UNNECESSARY_LOCK
