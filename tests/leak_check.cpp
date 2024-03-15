#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#ifdef _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif // _CRTDBG_MAP_ALLOC

class testRunListener : public Catch::EventListenerBase
{
  public:
    using Catch::EventListenerBase::EventListenerBase;

    void testRunStarting(Catch::TestRunInfo const &) override
    {
#ifdef _CRTDBG_MAP_ALLOC
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif // _CRTDBG_MAP_ALLOC
    }
};

CATCH_REGISTER_LISTENER(testRunListener)
