#include <catch2/catch_test_macros.hpp>

#include <wheels/allocators/cstdlib_allocator.hpp>
#include <wheels/containers/string.hpp>

#include "common.hpp"

using namespace wheels;

TEST_CASE("Create", "[String]")
{
    CstdlibAllocator allocator;

    {
        // Empty string should be a valid empty C-string with some memory
        // pre-allocated
        String str{allocator};
        REQUIRE(str.empty());
        REQUIRE(str.size() == 0);
        REQUIRE(str.capacity() > 0);
        REQUIRE(str.c_str() != nullptr);
        REQUIRE(str.c_str() == str.data());
        REQUIRE(strcmp(str.c_str(), "") == 0);
        REQUIRE(str.begin() == str.c_str());
        REQUIRE(str.begin() == str.end());

        String const &const_str = str;
        REQUIRE(const_str.c_str() == const_str.data());
        REQUIRE(const_str.begin() == const_str.c_str());
        REQUIRE(const_str.begin() == const_str.end());
    }

    {
        // String allocated explicitly empty should have capacity 1 store the
        // null
        String str{allocator, ""};
        REQUIRE(str.empty());
        REQUIRE(str.size() == 0);
        REQUIRE(str.capacity() == 0);
        REQUIRE(str.c_str() != nullptr);
        REQUIRE(str.c_str() == str.data());
        REQUIRE(strcmp(str.c_str(), "") == 0);
        REQUIRE(str.begin() == str.c_str());
        REQUIRE(str.begin() == str.end());
    }

    {
        // Initialized string should have the correct string allocated with the
        // exact required capacity
        char const ctest[] = "test";
        String str{allocator, ctest};
        REQUIRE(!str.empty());
        REQUIRE(str.size() == sizeof(ctest) - 1);
        REQUIRE(str.capacity() == sizeof(ctest) - 1);
        REQUIRE(str.c_str() != nullptr);
        REQUIRE(str.c_str() != ctest);
        REQUIRE(str.c_str() == str.data());
        REQUIRE(str.c_str()[sizeof(ctest) - 1] == '\0');
        REQUIRE(strcmp(str.c_str(), ctest) == 0);
        REQUIRE(str.begin() == str.c_str());
        REQUIRE(str.end() - str.begin() == sizeof(ctest) - 1);

        {
            String const &const_str = str;
            REQUIRE(const_str.c_str() == const_str.data());
            REQUIRE(const_str.begin() == const_str.c_str());
            REQUIRE(const_str.end() - const_str.begin() == sizeof(ctest) - 1);
        }

        char const *const str_alloc = str.c_str();

        // Moving should retain the same pointer
        String str_move_assigned{allocator};
        str_move_assigned = WHEELS_MOV(str);
        REQUIRE(!str_move_assigned.empty());
        REQUIRE(str_move_assigned.size() == sizeof(ctest) - 1);
        REQUIRE(str_move_assigned.capacity() == sizeof(ctest) - 1);
        REQUIRE(str_move_assigned.c_str() == str_alloc);
        REQUIRE(str_move_assigned.data() == str_alloc);

        String str_moved{WHEELS_MOV(str_move_assigned)};
        REQUIRE(str_moved.c_str() == str_alloc);
        REQUIRE(!str_moved.empty());
        REQUIRE(str_moved.size() == sizeof(ctest) - 1);
        REQUIRE(str_moved.capacity() == sizeof(ctest) - 1);
        REQUIRE(str_moved.c_str() == str_alloc);
        REQUIRE(str_moved.data() == str_alloc);
    }

    { // String from a span should have the correct content with exact size
        char const ctest[] = "test";
        String str{allocator, Span{ctest, sizeof(ctest) - 1}};
        REQUIRE(!str.empty());
        REQUIRE(str.size() == sizeof(ctest) - 1);
        REQUIRE(str.capacity() == sizeof(ctest) - 1);
        REQUIRE(str.c_str() != nullptr);
        REQUIRE(str.c_str() != ctest);
        REQUIRE(str.c_str()[sizeof(ctest) - 1] == '\0');
    }

    { // Span may not be a valid c-string and might not have anything allocated
      // at data()[size]. Result should still be a valid c-string. Also verify
      // that span to non-const works.
        char test[] = {'t', 'e', 's', 't'};
        String str{allocator, Span{test, sizeof(test)}};
        REQUIRE(!str.empty());
        REQUIRE(str.size() == sizeof(test));
        REQUIRE(str.capacity() == sizeof(test));
        REQUIRE(str.c_str() != nullptr);
        REQUIRE(str.c_str() != test);
        REQUIRE(str.c_str()[sizeof(test)] == '\0');
    }

    { // Trailing \0s in the span should be included
        char const ctest[] = "test\0\0";
        String str{allocator, Span{ctest, sizeof(ctest) - 1}};
        REQUIRE(!str.empty());
        REQUIRE(str.size() == sizeof(ctest) - 1);
        REQUIRE(str.capacity() == sizeof(ctest) - 1);
        REQUIRE(str.c_str() != nullptr);
        REQUIRE(str.c_str() != ctest);
        REQUIRE(str.c_str()[sizeof(ctest) - 1] == '\0');
        REQUIRE(memcmp(str.c_str(), ctest, sizeof(ctest)) == 0);
    }
}

TEST_CASE("Span", "[String]")
{
    CstdlibAllocator allocator;

    {
        // Empty string should be a valid pointer and size 0
        String str{allocator};
        Span<char const> span{str};
        REQUIRE(span.data() == str.c_str());
        REQUIRE(span.size() == 0);
    }

    {
        // String with content should be a valid pointer and size matching its
        // size
        String str{allocator, "test"};
        Span<char const> span{str};
        REQUIRE(span.data() == str.c_str());
        REQUIRE(span.size() == str.size());
    }

    {
        // Custom span should have the correct pointer and size
        String str{allocator, "test"};
        Span<char const> span = str.span(1, 3);
        REQUIRE(span.data() == str.c_str() + 1);
        REQUIRE(span.size() == 2);
    }
}

TEST_CASE("Access", "[String]")
{
    CstdlibAllocator allocator;

    char const tester[] = "tester";
    String str{allocator, tester};

    REQUIRE(str.front() == tester[0]);
    for (size_t i = 0; i < sizeof(tester) - 1; ++i)
        REQUIRE(str[i] == tester[i]);
    REQUIRE(str.back() == tester[sizeof(tester) - 2]);

    String const &const_str = str;
    REQUIRE(const_str.front() == tester[0]);
    for (size_t i = 0; i < sizeof(tester) - 1; ++i)
        REQUIRE(const_str[i] == tester[i]);
    REQUIRE(const_str.back() == tester[sizeof(tester) - 2]);

    {
        size_t i = 0;
        for (char c : str)
            REQUIRE(c == tester[i++]);
    }

    {
        size_t i = 0;
        for (char c : const_str)
            REQUIRE(c == tester[i++]);
    }
}

TEST_CASE("Comparisons", "[String]")
{
    CstdlibAllocator allocator;

    // Test comparisons against empty, matching, different sized, same sized
    // but not matching strings. Also test that strings with different amounts
    // of trailing nulls match. Assume that different capitalization and
    // encodings don't require special handling ie. just work^tm.

    char const ctest[] = "test";
    char const ctester[] = "tester";
    char const ctett[] = "tett";
    char const ctestnull[] = "test\0\0\0";
    String const empty{allocator};
    String const empty2{allocator};
    String const test{allocator, ctest};
    String const test2{allocator, ctest};
    String const tester{allocator, ctester};
    String const tett{allocator, ctett};
    String const testnull{allocator, ctestnull};

    REQUIRE("" == empty);
    REQUIRE(empty == "");
    REQUIRE(empty == empty);
    REQUIRE(empty == empty2);
    REQUIRE(empty2 == empty);
    REQUIRE(!(empty == ctest));
    REQUIRE(!(ctest == empty));
    REQUIRE(!(empty == test));
    REQUIRE(!(test == empty));

    REQUIRE(test == ctest);
    REQUIRE(ctest == test);
    REQUIRE(test == test);
    REQUIRE(test == test2);
    REQUIRE(test2 == test);
    REQUIRE(!(test == ctester));
    REQUIRE(!(ctester == test));
    REQUIRE(!(test == ctett));
    REQUIRE(!(ctett == test));
    REQUIRE(!(test == tester));
    REQUIRE(!(tester == test));
    REQUIRE(!(test == tett));
    REQUIRE(!(tett == test));

    REQUIRE(!("" != empty));
    REQUIRE(!(empty != ""));
    REQUIRE(!(empty != empty));
    REQUIRE(!(empty != empty2));
    REQUIRE(!(empty2 != empty));
    REQUIRE(empty != ctest);
    REQUIRE(ctest != empty);
    REQUIRE(empty != test);
    REQUIRE(test != empty);

    REQUIRE(!(test != ctest));
    REQUIRE(!(ctest != test));
    REQUIRE(!(test != test));
    REQUIRE(!(test != test2));
    REQUIRE(!(test2 != test));
    REQUIRE(test != ctester);
    REQUIRE(ctester != test);
    REQUIRE(test != ctett);
    REQUIRE(ctett != test);
    REQUIRE(test != tester);
    REQUIRE(tester != test);
    REQUIRE(test != tett);
    REQUIRE(tett != test);

    REQUIRE(testnull == ctest);
    REQUIRE(ctest == testnull);
    REQUIRE(testnull == test);
    REQUIRE(test == testnull);
    REQUIRE(!(testnull != ctest));
    REQUIRE(!(ctest != testnull));
    REQUIRE(!(testnull != test));
    REQUIRE(!(test != testnull));
}

TEST_CASE("Clear", "[String]")
{
    CstdlibAllocator allocator;

    // Clear should result in an empty c-string and size() 0. Allocation
    // shouldn't be affected.

    String empty{allocator};
    REQUIRE(empty.empty());
    REQUIRE(empty.size() == 0);
    REQUIRE(empty.capacity() > 0);
    REQUIRE(empty.begin() == empty.c_str());
    REQUIRE(empty.end() - empty.begin() == 0);
    size_t const capacity = empty.capacity();
    empty.clear();
    REQUIRE(empty.empty());
    REQUIRE(empty.size() == 0);
    REQUIRE(empty.capacity() == capacity);
    REQUIRE(empty.begin() == empty.c_str());
    REQUIRE(empty.end() - empty.begin() == 0);

    char const ctest[] = "test";
    String test{allocator, ctest};
    REQUIRE(!test.empty());
    REQUIRE(test.size() == sizeof(ctest) - 1);
    REQUIRE(test.capacity() == sizeof(ctest) - 1);
    REQUIRE(test.begin() == test.c_str());
    REQUIRE(test.end() - test.begin() == sizeof(ctest) - 1);
    test.clear();
    REQUIRE(test.empty());
    REQUIRE(test.size() == 0);
    REQUIRE(test.capacity() == sizeof(ctest) - 1);
    REQUIRE(test.begin() == test.c_str());
    REQUIRE(test.end() - test.begin() == 0);
}

TEST_CASE("Push/pop", "[String]")
{
    CstdlibAllocator allocator;

    // Push should append the new character, potentially allocating more space.
    // Pop should remove and return the last character.

    String str{allocator, ""};
    REQUIRE(str.empty());
    REQUIRE(str.capacity() == 0);
    str.push_back('\0');
    REQUIRE(!str.empty());
    REQUIRE(str.size() == 1);
    REQUIRE(str.capacity() == 1);
    REQUIRE(str.pop_back() == '\0');
    str.push_back('t');
    REQUIRE(!str.empty());
    REQUIRE(str.size() == 1);
    REQUIRE(str.capacity() == 1);
    REQUIRE(str.back() == 't');
    str.push_back('e');
    str.push_back('s');
    str.push_back('t');
    REQUIRE(str.size() == 4);
    REQUIRE(str.capacity() == 7);
    REQUIRE(str == "test");
    REQUIRE(str.pop_back() == 't');
    REQUIRE(str.size() == 3);
    REQUIRE(str == "tes");
    REQUIRE(str.pop_back() == 's');
    REQUIRE(str.size() == 2);
    REQUIRE(str == "te");
    REQUIRE(str.pop_back() == 'e');
    REQUIRE(str.size() == 1);
    REQUIRE(str == "t");
    REQUIRE(str.pop_back() == 't');
    REQUIRE(str.empty());
    REQUIRE(str.capacity() == 7);
}

TEST_CASE("Resize", "[String]")
{
    CstdlibAllocator allocator;

    // resize(n) should append \0s, resize(n, ch) should append ch.
    // n+1 should be \0 for both.
    // size() should be n after both.
    // resize to a smaller size should change only size and have \0 to make the
    // new length a valid c-string

    {
        String str{allocator, "test"};
        REQUIRE(str.size());
        REQUIRE(str.capacity() == 4);
        str.resize(7);
        REQUIRE(str.size() == 7);
        REQUIRE(str.capacity() == 7);
        REQUIRE(str == "test");
        REQUIRE(str[4] == '\0');
        REQUIRE(str[5] == '\0');
        REQUIRE(str[6] == '\0');
        REQUIRE(str.c_str()[7] == '\0');
        str.resize(3);
        REQUIRE(str.size() == 3);
        REQUIRE(str.capacity() == 7);
        REQUIRE(str == "tes");
        REQUIRE(str.c_str()[3] == '\0');
    }
}

TEST_CASE("Extend", "[String]")
{
    CstdlibAllocator allocator;

    { // Extend should append the string into the existing one with exact
      // capacity
        char const ctest[] = "test";
        String str{allocator, ctest};
        REQUIRE(str.size() == 4);
        REQUIRE(str.capacity() == 4);
        REQUIRE(str == ctest);
        str.extend(ctest);
        REQUIRE(str.size() == 8);
        REQUIRE(str.capacity() == 8);
        REQUIRE(str == "testtest");
        str.extend(Span{ctest, sizeof(ctest) - 1});
        REQUIRE(str.size() == 12);
        REQUIRE(str.capacity() == 12);
        REQUIRE(str == "testtesttest");
    }

    { // Extending with a span that has extra trailing nulls should append them
      // as well. Also verify that span to non-const works.
        char ctest[] = "test\0\0";
        String str{allocator, ctest};
        REQUIRE(str.size() == 4);
        REQUIRE(str.capacity() == 4);
        REQUIRE(str == "test");
        str.extend(Span{ctest, sizeof(ctest) - 1});
        REQUIRE(str.size() == 10);
        REQUIRE(str.capacity() == 10);
        REQUIRE(str == "testtest");
        REQUIRE(memcmp(str.c_str(), "testtest\0\0\0", str.size()) == 0);
    }
}

TEST_CASE("Find first", "[String]")
{
    CstdlibAllocator allocator;

    {
        char substr[] = ":;";
        { // Empty substr
            String str{allocator, "test"};

            // C-string
            REQUIRE(!str.find_first("").has_value());

            // Span
            Span<char const> const span{"", strlen("")};
            REQUIRE(!str.find_first(span).has_value());
        }

        { // Too short string
            String str{allocator, "t"};

            // C-string
            REQUIRE(!str.find_first(substr).has_value());

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(!str.find_first(span).has_value());
        }

        { // Empty string
            String str{allocator, ""};

            // C-string
            REQUIRE(!str.find_first(substr).has_value());

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(!str.find_first(span).has_value());
        }

        { // Not found
            String str{allocator, "test"};

            // C-string
            REQUIRE(!str.find_first(substr).has_value());

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(!str.find_first(span).has_value());
        }

        { // Front
            String str{allocator, ":;test"};

            // C-string
            REQUIRE(str.find_first(substr).has_value());
            REQUIRE(*str.find_first(substr) == 0);

            { // Span to const
                Span<char const> const span{substr, strlen(substr)};
                REQUIRE(str.find_first(span).has_value());
                REQUIRE(*str.find_first(span) == 0);
            }
            { // Span to non-const
                Span<char> const span{substr, strlen(substr)};
                REQUIRE(str.find_first(span).has_value());
                REQUIRE(*str.find_first(span) == 0);
            }
        }

        { // Middle
            String str{allocator, "te:;st"};

            // C-string
            REQUIRE(str.find_first(substr).has_value());
            REQUIRE(*str.find_first(substr) == 2);

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(str.find_first(span).has_value());
            REQUIRE(*str.find_first(span) == 2);
        }

        { // End
            String str{allocator, "test:;"};

            // C-string
            REQUIRE(str.find_first(substr).has_value());
            REQUIRE(*str.find_first(substr) == 4);

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(str.find_first(span).has_value());
            REQUIRE(*str.find_first(span) == 4);
        }

        { // Multiple
            String str{allocator, "te:;st:;"};

            // C-string
            REQUIRE(str.find_first(substr).has_value());
            REQUIRE(*str.find_first(substr) == 2);

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(str.find_first(span).has_value());
            REQUIRE(*str.find_first(span) == 2);
        }

        { // Before middle \0
            String str{allocator, ":;test"};
            str[4] = '\0';

            // C-string
            REQUIRE(str.find_first(substr).has_value());
            REQUIRE(*str.find_first(substr) == 0);

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(str.find_first(span).has_value());
            REQUIRE(*str.find_first(span) == 0);
        }

        { // After middle \0
            String str{allocator, "test:;"};
            str[2] = '\0';

            // C-string
            REQUIRE(str.find_first(substr).has_value());
            REQUIRE(*str.find_first(substr) == 4);

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(str.find_first(span).has_value());
            REQUIRE(*str.find_first(span) == 4);
        }
    }

    {
        char const ch = ':';
        { // Empty
            String str{allocator, ""};
            REQUIRE(!str.find_first(ch).has_value());
        }

        { // Not found
            String str{allocator, "test"};
            REQUIRE(!str.find_first(ch).has_value());
        }

        { // Front
            String str{allocator, ":test"};
            REQUIRE(str.find_first(ch).has_value());
            REQUIRE(*str.find_first(ch) == 0);
        }

        { // Back
            String str{allocator, "test:"};
            REQUIRE(str.find_first(ch).has_value());
            REQUIRE(*str.find_first(ch) == 4);
        }

        { // Middle
            String str{allocator, "te:st"};
            REQUIRE(str.find_first(ch).has_value());
            REQUIRE(*str.find_first(ch) == 2);
        }

        { // Multiple
            String str{allocator, "te:st:"};
            REQUIRE(str.find_first(ch).has_value());
            REQUIRE(*str.find_first(ch) == 2);
        }

        { // Before middle \0
            String str{allocator, ":test"};
            str[3] = '\0';
            REQUIRE(str.find_first(ch).has_value());
            REQUIRE(*str.find_first(ch) == 0);
        }

        { // After middle \0
            String str{allocator, "test:"};
            str[2] = '\0';
            REQUIRE(str.find_first(ch).has_value());
            REQUIRE(*str.find_first(ch) == 4);
        }
    }
}

TEST_CASE("Find last", "[String]")
{
    CstdlibAllocator allocator;

    {
        char substr[] = ":;";
        { // Empty substr
            String str{allocator, "test"};

            // C-string
            REQUIRE(!str.find_last("").has_value());

            // Span
            Span<char const> const span{"", strlen("")};
            REQUIRE(!str.find_last(span).has_value());
        }

        { // Too short string
            String str{allocator, "t"};

            // C-string
            REQUIRE(!str.find_last(substr).has_value());

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(!str.find_last(span).has_value());
        }

        { // Empty string
            String str{allocator, ""};

            // C-string
            REQUIRE(!str.find_last(substr).has_value());

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(!str.find_last(span).has_value());
        }

        { // Not found
            String str{allocator, "test"};

            // C-string
            REQUIRE(!str.find_last(substr).has_value());

            { // Span to const
                Span<char const> const span{substr, strlen(substr)};
                REQUIRE(!str.find_last(span).has_value());
            }
            { // Span to non-const
                Span<char> const span{substr, strlen(substr)};
                REQUIRE(!str.find_last(span).has_value());
            }
        }

        { // Front
            String str{allocator, ":;test"};

            // C-string
            REQUIRE(str.find_last(substr).has_value());
            REQUIRE(*str.find_last(substr) == 0);

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(str.find_last(span).has_value());
            REQUIRE(*str.find_last(span) == 0);
        }

        { // Middle
            String str{allocator, "te:;st"};

            // C-string
            REQUIRE(str.find_last(substr).has_value());
            REQUIRE(*str.find_last(substr) == 2);

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(str.find_last(span).has_value());
            REQUIRE(*str.find_last(span) == 2);
        }

        { // End
            String str{allocator, "test:;"};

            // C-string
            REQUIRE(str.find_last(substr).has_value());
            REQUIRE(*str.find_last(substr) == 4);

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(str.find_last(span).has_value());
            REQUIRE(*str.find_last(span) == 4);
        }

        { // Multiple
            String str{allocator, ":;te:;st"};

            // C-string
            REQUIRE(str.find_last(substr).has_value());
            REQUIRE(*str.find_last(substr) == 4);

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(str.find_last(span).has_value());
            REQUIRE(*str.find_last(span) == 4);
        }

        { // Before middle \0
            String str{allocator, ":;test"};
            str[4] = '\0';

            // C-string
            REQUIRE(str.find_last(substr).has_value());
            REQUIRE(*str.find_last(substr) == 0);

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(str.find_last(span).has_value());
            REQUIRE(*str.find_last(span) == 0);
        }

        { // After middle \0
            String str{allocator, "tes:;t"};
            str[2] = '\0';

            // C-string
            REQUIRE(str.find_last(substr).has_value());
            REQUIRE(*str.find_last(substr) == 3);

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(str.find_last(span).has_value());
            REQUIRE(*str.find_last(span) == 3);
        }
    }

    {
        char const ch = ':';
        { // Empty
            String str{allocator, ""};
            REQUIRE(!str.find_first(ch).has_value());
        }

        { // Not found
            String str{allocator, "test"};
            REQUIRE(!str.find_last(ch).has_value());
        }

        { // Front
            String str{allocator, ":test"};
            REQUIRE(str.find_last(ch).has_value());
            REQUIRE(*str.find_last(ch) == 0);
        }

        { // Back
            String str{allocator, "test:"};
            REQUIRE(str.find_last(ch).has_value());
            REQUIRE(*str.find_last(ch) == 4);
        }

        { // Middle
            String str{allocator, "te:st"};
            REQUIRE(str.find_last(ch).has_value());
            REQUIRE(*str.find_last(ch) == 2);
        }

        { // Multiple
            String str{allocator, ":te:st"};
            REQUIRE(str.find_last(ch).has_value());
            REQUIRE(*str.find_last(ch) == 3);
        }

        { // Before middle \0
            String str{allocator, "t:est"};
            str[2] = '\0';
            REQUIRE(str.find_last(ch).has_value());
            REQUIRE(*str.find_last(ch) == 1);
        }

        { // After middle \0
            String str{allocator, "tes:t"};
            str[2] = '\0';
            REQUIRE(str.find_last(ch).has_value());
            REQUIRE(*str.find_last(ch) == 3);
        }
    }
}

TEST_CASE("Contains", "[String]")
{
    CstdlibAllocator allocator;

    {
        char substr[] = ":;";
        { // Empty substr
            String str{allocator, "test"};

            // C-string
            REQUIRE(!str.contains(""));

            // Span
            Span<char const> const span{"", strlen("")};
            REQUIRE(!str.contains(span));
        }

        { // Too short string
            String str{allocator, "t"};

            // C-string
            REQUIRE(!str.contains(substr));

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(!str.contains(span));
        }

        { // Empty string
            String str{allocator, ""};

            // C-string
            REQUIRE(!str.contains(substr));

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(!str.contains(span));
        }

        { // Not found
            String str{allocator, "test"};

            // C-string
            REQUIRE(!str.contains(substr));

            { // Span to const
                Span<char const> const span{substr, strlen(substr)};
                REQUIRE(!str.contains(span));
            }
            { // Span to non-const
                Span<char> const span{substr, strlen(substr)};
                REQUIRE(!str.contains(span));
            }
        }

        { // Front
            String str{allocator, ":;test"};

            // C-string
            REQUIRE(str.contains(substr));

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(str.contains(span));
        }

        { // Middle
            String str{allocator, "te:;st"};

            // C-string
            REQUIRE(str.contains(substr));

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(str.contains(span));
        }

        { // End
            String str{allocator, "test:;"};

            // C-string
            REQUIRE(str.contains(substr));

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(str.contains(span));
        }

        { // Multiple
            String str{allocator, "te:;st:;"};

            // C-string
            REQUIRE(str.contains(substr));

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(str.contains(span));
        }

        { // Before middle \0
            String str{allocator, ":;test"};
            str[4] = '\0';

            // C-string
            REQUIRE(str.contains(substr));

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(str.contains(span));
        }

        { // After middle \0
            String str{allocator, "test:;"};
            str[2] = '\0';

            // C-string
            REQUIRE(str.contains(substr));

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(str.contains(span));
        }
    }

    {
        char const ch = ':';
        { // Empty
            String str{allocator, ""};
            REQUIRE(!str.contains(ch));
        }

        { // Not found
            String str{allocator, "test"};
            REQUIRE(!str.contains(ch));
        }

        { // Front
            String str{allocator, ":test"};
            REQUIRE(str.contains(ch));
        }

        { // Back
            String str{allocator, "test:"};
            REQUIRE(str.contains(ch));
        }

        { // Middle
            String str{allocator, "te:st"};
            REQUIRE(str.contains(ch));
        }

        { // Multiple
            String str{allocator, "te:st:"};
            REQUIRE(str.contains(ch));
        }

        { // Before middle \0
            String str{allocator, ":test"};
            str[3] = '\0';
            REQUIRE(str.contains(ch));
        }

        { // After middle \0
            String str{allocator, "test:"};
            str[2] = '\0';
            REQUIRE(str.contains(ch));
        }
    }
}

TEST_CASE("Starts with", "[String]")
{
    CstdlibAllocator allocator;

    {
        char substr[] = ":;";
        { // Empty substr
            String str{allocator, "test"};

            // C-string
            REQUIRE(!str.starts_with(""));

            // Span
            Span<char const> const span{"", strlen("")};
            REQUIRE(!str.starts_with(span));
        }

        { // Too short string
            String str{allocator, "t"};

            // C-string
            REQUIRE(!str.starts_with(substr));

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(!str.starts_with(span));
        }

        { // Empty string
            String str{allocator, ""};

            // C-string
            REQUIRE(!str.starts_with(substr));

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(!str.starts_with(span));
        }

        { // Not found
            String str{allocator, "test"};

            // C-string
            REQUIRE(!str.starts_with(substr));

            { // Span to const
                Span<char const> const span{substr, strlen(substr)};
                REQUIRE(!str.starts_with(span));
            }
            { // Span to non-const
                Span<char> const span{substr, strlen(substr)};
                REQUIRE(!str.starts_with(span));
            }
        }

        { // Front
            String str{allocator, ":;test"};

            // C-string
            REQUIRE(str.starts_with(substr));

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(str.starts_with(span));
        }

        { // Middle
            String str{allocator, "te:;st"};

            // C-string
            REQUIRE(!str.starts_with(substr));

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(!str.starts_with(span));
        }

        { // End
            String str{allocator, "test:;"};

            // C-string
            REQUIRE(!str.starts_with(substr));

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(!str.starts_with(span));
        }

        { // After middle \0
            String str{allocator, "test:;"};
            str[3] = '\0';

            // C-string
            REQUIRE(!str.starts_with(substr));

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(!str.starts_with(span));
        }
    }

    {
        char const ch = ':';
        { // None
            String str{allocator, "test"};
            REQUIRE(!str.starts_with(ch));
        }

        { // Front
            String str{allocator, ":test"};
            REQUIRE(str.starts_with(ch));
        }

        { // Back
            String str{allocator, "test:"};
            REQUIRE(!str.starts_with(ch));
        }

        { // Middle
            String str{allocator, "te:st"};
            REQUIRE(!str.starts_with(ch));
        }

        { // After middle \0
            String str{allocator, "test:"};
            str[3] = '\0';
            REQUIRE(!str.starts_with(ch));
        }
    }
}

TEST_CASE("Ends with", "[String]")
{
    CstdlibAllocator allocator;

    {
        char substr[] = ":;";
        { // Empty substr
            String str{allocator, "test"};

            // C-string
            REQUIRE(!str.ends_with(""));

            // Span
            Span<char const> const span{"", strlen("")};
            REQUIRE(!str.ends_with(span));
        }

        { // Too short string
            String str{allocator, "t"};

            // C-string
            REQUIRE(!str.ends_with(substr));

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(!str.ends_with(span));
        }

        { // Empty string
            String str{allocator, ""};

            // C-string
            REQUIRE(!str.ends_with(substr));

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(!str.ends_with(span));
        }

        { // Not found
            String str{allocator, "test"};

            // C-string
            REQUIRE(!str.ends_with(substr));

            { // Span to const
                Span<char const> const span{substr, strlen(substr)};
                REQUIRE(!str.ends_with(span));
            }
            { // Span to non-const
                Span<char> const span{substr, strlen(substr)};
                REQUIRE(!str.ends_with(span));
            }
        }

        { // Front
            String str{allocator, ":;test"};

            // C-string
            REQUIRE(!str.ends_with(substr));

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(!str.ends_with(span));
        }

        { // Middle
            String str{allocator, "te:;st"};

            // C-string
            REQUIRE(!str.ends_with(substr));

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(!str.ends_with(span));
        }

        { // End
            String str{allocator, "test:;"};

            // C-string
            REQUIRE(str.ends_with(substr));

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(str.ends_with(span));
        }

        { // Before middle \0
            String str{allocator, "t:;est"};
            str[3] = '\0';

            // C-string
            REQUIRE(!str.ends_with(substr));

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(!str.ends_with(span));
        }

        { // After middle \0
            String str{allocator, "test:;"};
            str[2] = '\0';

            // C-string
            REQUIRE(str.ends_with(substr));

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(str.ends_with(span));
        }
    }

    {
        char const ch = ':';
        { // None
            String str{allocator, "test"};
            REQUIRE(!str.ends_with(ch));
        }

        { // Front
            String str{allocator, ":test"};
            REQUIRE(!str.ends_with(ch));
        }

        { // Back
            char const ch = ':';
            String str{allocator, "test:"};
            REQUIRE(str.ends_with(ch));
        }

        { // Middle
            String str{allocator, "te:st"};
            REQUIRE(!str.ends_with(ch));
        }

        { // Before middle \0
            String str{allocator, "te:st"};
            str[3] = '\0';
            REQUIRE(!str.ends_with(ch));
        }

        { // After middle \0
            String str{allocator, "test:"};
            str[2] = '\0';
            REQUIRE(str.ends_with(ch));
        }
    }
}

TEST_CASE("Split", "[String]")
{
    CstdlibAllocator allocator;

    {
        char substr[] = ":;";
        { // Empty substr
            String str{allocator, "test"};

            { // C-string
                Array<Span<char const>> split = str.split(allocator, "");
                REQUIRE(split.size() == 1);
                REQUIRE(split[0].data() == str.c_str());
                REQUIRE(split[0].size() == str.size());
                REQUIRE(
                    memcmp(split[0].data(), "test", sizeof("test") - 1) == 0);
            }
            { // Span
                Span<char const> const span{"", strlen("")};
                Array<Span<char const>> split = str.split(allocator, span);
                REQUIRE(split.size() == 1);
                REQUIRE(split[0].data() == str.c_str());
                REQUIRE(split[0].size() == str.size());
                REQUIRE(
                    memcmp(split[0].data(), "test", sizeof("test") - 1) == 0);
            }
        }

        { // Too short string
            String str{allocator, "t"};

            { // C-string
                Array<Span<char const>> split = str.split(allocator, "");
                REQUIRE(split.size() == 1);
                REQUIRE(split[0].data() == str.c_str());
                REQUIRE(split[0].size() == str.size());
                REQUIRE(memcmp(split[0].data(), "t", sizeof("t") - 1) == 0);
            }
            { // Span
                Span<char const> const span{"", strlen("")};
                Array<Span<char const>> split = str.split(allocator, span);
                REQUIRE(split.size() == 1);
                REQUIRE(split[0].data() == str.c_str());
                REQUIRE(split[0].size() == str.size());
                REQUIRE(memcmp(split[0].data(), "t", sizeof("t") - 1) == 0);
            }
        }

        { // Empty string
            String str{allocator, ""};

            // C-string
            REQUIRE(str.split(allocator, substr).empty());

            // Span
            Span<char const> const span{substr, strlen(substr)};
            REQUIRE(str.split(allocator, span).empty());
        }

        { // Not found
            String str{allocator, "test"};

            { // C-string
                Array<Span<char const>> split = str.split(allocator, substr);
                REQUIRE(split.size() == 1);
                REQUIRE(split[0].data() == str.c_str());
                REQUIRE(split[0].size() == str.size());
                REQUIRE(
                    memcmp(split[0].data(), "test", sizeof("test") - 1) == 0);
            }
            { // Span to const
                Span<char const> const span{substr, strlen(substr)};
                Array<Span<char const>> split = str.split(allocator, span);
                REQUIRE(split.size() == 1);
                REQUIRE(split[0].data() == str.c_str());
                REQUIRE(split[0].size() == str.size());
                REQUIRE(
                    memcmp(split[0].data(), "test", sizeof("test") - 1) == 0);
            }
            { // Span to non-const
                Span<char> const span{substr, strlen(substr)};
                Array<Span<char const>> split = str.split(allocator, span);
                REQUIRE(split.size() == 1);
                REQUIRE(split[0].data() == str.c_str());
                REQUIRE(split[0].size() == str.size());
                REQUIRE(
                    memcmp(split[0].data(), "test", sizeof("test") - 1) == 0);
            }
        }

        { // Front
            String str{allocator, ":;test"};

            { // C-string
                Array<Span<char const>> split = str.split(allocator, substr);
                REQUIRE(split.size() == 1);
                REQUIRE(split[0].data() == str.c_str() + 2);
                REQUIRE(split[0].size() == str.size() - 2);
                REQUIRE(
                    memcmp(split[0].data(), "test", sizeof("test") - 1) == 0);
            }
            { // Span
                Span<char const> const span{substr, strlen(substr)};
                Array<Span<char const>> split = str.split(allocator, span);
                REQUIRE(split.size() == 1);
                REQUIRE(split[0].data() == str.c_str() + 2);
                REQUIRE(split[0].size() == str.size() - 2);
                REQUIRE(
                    memcmp(split[0].data(), "test", sizeof("test") - 1) == 0);
            }
        }

        { // Middle
            String str{allocator, "te:;st"};

            { // C-string
                Array<Span<char const>> split = str.split(allocator, substr);
                REQUIRE(split.size() == 2);
                REQUIRE(split[0].data() == str.c_str());
                REQUIRE(split[0].size() == 2);
                REQUIRE(memcmp(split[0].data(), "te", sizeof("te") - 1) == 0);
                REQUIRE(split[1].data() == str.c_str() + 4);
                REQUIRE(split[1].size() == 2);
                REQUIRE(memcmp(split[1].data(), "st", sizeof("st") - 1) == 0);
            }
            { // Span
                Span<char const> const span{substr, strlen(substr)};
                Array<Span<char const>> split = str.split(allocator, span);
                REQUIRE(split.size() == 2);
                REQUIRE(split[0].data() == str.c_str());
                REQUIRE(split[0].size() == 2);
                REQUIRE(memcmp(split[0].data(), "te", sizeof("te") - 1) == 0);
                REQUIRE(split[1].data() == str.c_str() + 4);
                REQUIRE(split[1].size() == 2);
                REQUIRE(memcmp(split[1].data(), "st", sizeof("st") - 1) == 0);
            }
        }

        { // End
            String str{allocator, "test:;"};

            { // C-string
                Array<Span<char const>> split = str.split(allocator, substr);
                REQUIRE(split.size() == 1);
                REQUIRE(split[0].data() == str.c_str());
                REQUIRE(split[0].size() == 4);
                REQUIRE(
                    memcmp(split[0].data(), "test", sizeof("test") - 1) == 0);
            }
            { // Span
                Span<char const> const span{substr, strlen(substr)};
                Array<Span<char const>> split = str.split(allocator, span);
                REQUIRE(split.size() == 1);
                REQUIRE(split[0].data() == str.c_str());
                REQUIRE(split[0].size() == 4);
                REQUIRE(
                    memcmp(split[0].data(), "test", sizeof("test") - 1) == 0);
            }
        }

        { // Multiple
            String str{allocator, ":;te:;st"};

            { // C-string
                Array<Span<char const>> split = str.split(allocator, substr);
                REQUIRE(split.size() == 2);
                REQUIRE(split[0].data() == str.c_str() + 2);
                REQUIRE(split[0].size() == 2);
                REQUIRE(memcmp(split[0].data(), "te", sizeof("te") - 1) == 0);
                REQUIRE(split[1].data() == str.c_str() + 6);
                REQUIRE(split[1].size() == 2);
                REQUIRE(memcmp(split[1].data(), "st", sizeof("st") - 1) == 0);
            }
            { // Span
                Span<char const> const span{substr, strlen(substr)};
                Array<Span<char const>> split = str.split(allocator, span);
                REQUIRE(split.size() == 2);
                REQUIRE(split[0].data() == str.c_str() + 2);
                REQUIRE(split[0].size() == 2);
                REQUIRE(memcmp(split[0].data(), "te", sizeof("te") - 1) == 0);
                REQUIRE(split[1].data() == str.c_str() + 6);
                REQUIRE(split[1].size() == 2);
                REQUIRE(memcmp(split[1].data(), "st", sizeof("st") - 1) == 0);
            }
        }

        { // Multiple back-to-back front
            String str{allocator, ":;:;test"};

            { // C-string
                Array<Span<char const>> split = str.split(allocator, substr);
                REQUIRE(split.size() == 1);
                REQUIRE(split[0].data() == str.c_str() + 4);
                REQUIRE(split[0].size() == 4);
                REQUIRE(
                    memcmp(split[0].data(), "test", sizeof("test") - 1) == 0);
            }
            { // Span
                Span<char const> const span{substr, strlen(substr)};
                Array<Span<char const>> split = str.split(allocator, span);
                REQUIRE(split.size() == 1);
                REQUIRE(split[0].data() == str.c_str() + 4);
                REQUIRE(split[0].size() == 4);
                REQUIRE(
                    memcmp(split[0].data(), "test", sizeof("test") - 1) == 0);
            }
        }

        { // Multiple back-to-back middle
            String str{allocator, "te:;:;st"};

            { // C-string
                Array<Span<char const>> split = str.split(allocator, substr);
                REQUIRE(split.size() == 2);
                REQUIRE(split[0].data() == str.c_str());
                REQUIRE(split[0].size() == 2);
                REQUIRE(memcmp(split[0].data(), "te", sizeof("te") - 1) == 0);
                REQUIRE(split[1].data() == str.c_str() + 6);
                REQUIRE(split[1].size() == 2);
                REQUIRE(memcmp(split[1].data(), "st", sizeof("st") - 1) == 0);
            }
            { // Span
                Span<char const> const span{substr, strlen(substr)};
                Array<Span<char const>> split = str.split(allocator, span);
                REQUIRE(split.size() == 2);
                REQUIRE(split[0].data() == str.c_str());
                REQUIRE(split[0].size() == 2);
                REQUIRE(memcmp(split[0].data(), "te", sizeof("te") - 1) == 0);
                REQUIRE(split[1].data() == str.c_str() + 6);
                REQUIRE(split[1].size() == 2);
                REQUIRE(memcmp(split[1].data(), "st", sizeof("st") - 1) == 0);
            }
        }

        { // Multiple back-to-back end
            String str{allocator, "test:;:;"};

            { // C-string
                Array<Span<char const>> split = str.split(allocator, substr);
                REQUIRE(split.size() == 1);
                REQUIRE(split[0].data() == str.c_str());
                REQUIRE(split[0].size() == 4);
                REQUIRE(
                    memcmp(split[0].data(), "test", sizeof("test") - 1) == 0);
            }
            { // Span
                Span<char const> const span{substr, strlen(substr)};
                Array<Span<char const>> split = str.split(allocator, span);
                REQUIRE(split.size() == 1);
                REQUIRE(split[0].data() == str.c_str());
                REQUIRE(split[0].size() == 4);
                REQUIRE(
                    memcmp(split[0].data(), "test", sizeof("test") - 1) == 0);
            }
        }

        { // Before middle \0
            String str{allocator, "te:;st"};
            str[4] = '\0';

            { // C-string
                Array<Span<char const>> split = str.split(allocator, substr);
                REQUIRE(split.size() == 2);
                REQUIRE(split[0].data() == str.c_str());
                REQUIRE(split[0].size() == 2);
                REQUIRE(memcmp(split[0].data(), "te", sizeof("te") - 1) == 0);
                REQUIRE(split[1].data() == str.c_str() + 4);
                REQUIRE(split[1].size() == 2);
                REQUIRE(memcmp(split[1].data(), "\0t", sizeof("\0t") - 1) == 0);
            }
            { // Span
                Span<char const> const span{substr, strlen(substr)};
                Array<Span<char const>> split = str.split(allocator, span);
                REQUIRE(split.size() == 2);
                REQUIRE(split[0].data() == str.c_str());
                REQUIRE(split[0].size() == 2);
                REQUIRE(memcmp(split[0].data(), "te", sizeof("te") - 1) == 0);
                REQUIRE(split[1].data() == str.c_str() + 4);
                REQUIRE(split[1].size() == 2);
                REQUIRE(memcmp(split[1].data(), "\0t", sizeof("\0t") - 1) == 0);
            }
        }

        { // After middle \0
            String str{allocator, "tes:;t"};
            str[2] = '\0';

            { // C-string
                Array<Span<char const>> split = str.split(allocator, substr);
                REQUIRE(split.size() == 2);
                REQUIRE(split[0].data() == str.c_str());
                REQUIRE(split[0].size() == 3);
                REQUIRE(
                    memcmp(split[0].data(), "te\0", sizeof("te\0") - 1) == 0);
                REQUIRE(split[1].data() == str.c_str() + 5);
                REQUIRE(split[1].size() == 1);
                REQUIRE(memcmp(split[1].data(), "t", sizeof("t") - 1) == 0);
            }
            { // Span
                Span<char const> const span{substr, strlen(substr)};
                Array<Span<char const>> split = str.split(allocator, span);
                REQUIRE(split.size() == 2);
                REQUIRE(split[0].data() == str.c_str());
                REQUIRE(split[0].size() == 3);
                REQUIRE(
                    memcmp(split[0].data(), "te\0", sizeof("te\0") - 1) == 0);
                REQUIRE(split[1].data() == str.c_str() + 5);
                REQUIRE(split[1].size() == 1);
                REQUIRE(memcmp(split[1].data(), "t", sizeof("t") - 1) == 0);
            }
        }
    }

    {
        char const ch = ':';
        { // None
            String str{allocator, "test"};
            Array<Span<char const>> split = str.split(allocator, ch);
            REQUIRE(split.size() == 1);
            REQUIRE(split[0].data() == str.c_str());
            REQUIRE(split[0].size() == str.size());
            REQUIRE(memcmp(split[0].data(), "test", sizeof("test") - 1) == 0);
        }

        { // Front
            String str{allocator, ":test"};
            Array<Span<char const>> split = str.split(allocator, ch);
            REQUIRE(split.size() == 1);
            REQUIRE(split[0].data() == str.c_str() + 1);
            REQUIRE(split[0].size() == 4);
            REQUIRE(memcmp(split[0].data(), "test", sizeof("test") - 1) == 0);
        }

        { // Back
            String str{allocator, "test:"};
            Array<Span<char const>> split = str.split(allocator, ch);
            REQUIRE(split.size() == 1);
            REQUIRE(split[0].data() == str.c_str());
            REQUIRE(split[0].size() == 4);
            REQUIRE(memcmp(split[0].data(), "test", sizeof("test") - 1) == 0);
        }

        { // Middle
            String str{allocator, "te:st"};
            Array<Span<char const>> split = str.split(allocator, ch);
            REQUIRE(split.size() == 2);
            REQUIRE(split[0].data() == str.c_str());
            REQUIRE(split[0].size() == 2);
            REQUIRE(memcmp(split[0].data(), "te", sizeof("te") - 1) == 0);
            REQUIRE(split[1].data() == str.c_str() + 3);
            REQUIRE(split[1].size() == 2);
            REQUIRE(memcmp(split[1].data(), "st", sizeof("st") - 1) == 0);
        }

        { // Multiple
            String str{allocator, ":te:st"};
            Array<Span<char const>> split = str.split(allocator, ch);
            REQUIRE(split.size() == 2);
            REQUIRE(split[0].data() == str.c_str() + 1);
            REQUIRE(split[0].size() == 2);
            REQUIRE(memcmp(split[0].data(), "te", sizeof("te") - 1) == 0);
            REQUIRE(split[1].data() == str.c_str() + 4);
            REQUIRE(split[1].size() == 2);
            REQUIRE(memcmp(split[1].data(), "st", sizeof("st") - 1) == 0);
        }

        { // Multiple back-to-back front
            String str{allocator, "::test"};
            Array<Span<char const>> split = str.split(allocator, ch);
            REQUIRE(split.size() == 1);
            REQUIRE(split[0].data() == str.c_str() + 2);
            REQUIRE(split[0].size() == 4);
            REQUIRE(memcmp(split[0].data(), "test", sizeof("test") - 1) == 0);
        }

        { // Multiple back-to-back middle
            String str{allocator, "te::st"};
            Array<Span<char const>> split = str.split(allocator, ch);
            REQUIRE(split.size() == 2);
            REQUIRE(split[0].data() == str.c_str());
            REQUIRE(split[0].size() == 2);
            REQUIRE(memcmp(split[0].data(), "te", sizeof("te") - 1) == 0);
            REQUIRE(split[1].data() == str.c_str() + 4);
            REQUIRE(split[1].size() == 2);
            REQUIRE(memcmp(split[1].data(), "st", sizeof("st") - 1) == 0);
        }

        { // Multiple back-to-back end
            String str{allocator, "test::"};
            Array<Span<char const>> split = str.split(allocator, ch);
            REQUIRE(split.size() == 1);
            REQUIRE(split[0].data() == str.c_str());
            REQUIRE(split[0].size() == 4);
            REQUIRE(memcmp(split[0].data(), "test", sizeof("test") - 1) == 0);
        }

        { // Before middle \0
            String str{allocator, "t:est"};
            str[2] = '\0';
            Array<Span<char const>> split = str.split(allocator, ch);
            REQUIRE(split.size() == 2);
            REQUIRE(split[0].data() == str.c_str());
            REQUIRE(split[0].size() == 1);
            REQUIRE(memcmp(split[0].data(), "t", sizeof("t") - 1) == 0);
            REQUIRE(split[1].data() == str.c_str() + 2);
            REQUIRE(split[1].size() == 3);
            REQUIRE(memcmp(split[1].data(), "\0st", sizeof("\0st") - 1) == 0);
        }

        { // After middle \0
            String str{allocator, "tes:t"};
            str[2] = '\0';
            Array<Span<char const>> split = str.split(allocator, ch);
            REQUIRE(split.size() == 2);
            REQUIRE(split[0].data() == str.c_str());
            REQUIRE(split[0].size() == 3);
            REQUIRE(memcmp(split[0].data(), "te\0", sizeof("te\0") - 1) == 0);
            REQUIRE(split[1].data() == str.c_str() + 4);
            REQUIRE(split[1].size() == 1);
            REQUIRE(memcmp(split[1].data(), "t", sizeof("t") - 1) == 0);
        }
    }
}

TEST_CASE("Concat", "[String]")
{
    CstdlibAllocator allocator;

    // Concat should allocate a new string with the content of the content of
    // the second argument appended to the first's.
    String empty{allocator};
    char cempty[] = "";
    String foo{allocator, "foo"};
    String bara{allocator, "bara"};
    String ba0a{allocator, "bara"};
    ba0a[2] = '\0';
    char cfoo[] = "foo";
    char cbara[] = "bara";

    {
        String str = concat(allocator, empty, cempty);
        REQUIRE(str.empty());
        REQUIRE(str.capacity() == 16); // Default size
        REQUIRE(str.data()[0] == '\0');
    }

    {
        String str = concat(allocator, cempty, empty);
        REQUIRE(str.empty());
        REQUIRE(str.capacity() == 16); // Default size
        REQUIRE(str.data()[0] == '\0');
    }

    {
        String str = concat(allocator, empty, empty);
        REQUIRE(str.empty());
        REQUIRE(str.capacity() == 16); // Default size
        REQUIRE(str.data()[0] == '\0');
    }

    {
        String str = concat(allocator, foo, bara);
        REQUIRE(!str.empty());
        REQUIRE(str.size() == 7);
        REQUIRE(str.capacity() == 7);
        REQUIRE(str.data()[7] == '\0');
        REQUIRE(str == "foobara");
    }

    {
        String str = concat(allocator, foo, cbara);
        REQUIRE(!str.empty());
        REQUIRE(str.size() == 7);
        REQUIRE(str.capacity() == 7);
        REQUIRE(str.data()[7] == '\0');
        REQUIRE(str == "foobara");
    }

    {
        String str = concat(allocator, cfoo, bara);
        REQUIRE(!str.empty());
        REQUIRE(str.size() == 7);
        REQUIRE(str.capacity() == 7);
        REQUIRE(str.data()[7] == '\0');
        REQUIRE(str == "foobara");
    }

    {
        String str = concat(allocator, foo, ba0a);
        REQUIRE(!str.empty());
        REQUIRE(str.size() == 7);
        REQUIRE(str.capacity() == 7);
        REQUIRE(str.data()[7] == '\0');
        REQUIRE(str == "fooba\0a");
    }

    {
        String str = concat(allocator, cfoo, ba0a);
        REQUIRE(!str.empty());
        REQUIRE(str.size() == 7);
        REQUIRE(str.capacity() == 7);
        REQUIRE(str.data()[7] == '\0');
        REQUIRE(str == "fooba\0a");
    }

    {
        String str = concat(allocator, ba0a, cfoo);
        REQUIRE(!str.empty());
        REQUIRE(str.size() == 7);
        REQUIRE(str.capacity() == 7);
        REQUIRE(str.data()[7] == '\0');
        REQUIRE(str == "ba\0afoo");
    }

    {
        String str = concat(allocator, ba0a, foo);
        REQUIRE(!str.empty());
        REQUIRE(str.size() == 7);
        REQUIRE(str.capacity() == 7);
        REQUIRE(str.data()[7] == '\0');
        REQUIRE(str == "ba\0afoo");
    }

    {
        String str = concat(allocator, foo, empty);
        REQUIRE(!str.empty());
        REQUIRE(str.size() == 3);
        REQUIRE(str.capacity() == 3);
        REQUIRE(str.data()[3] == '\0');
        REQUIRE(str == "foo");
    }

    {
        String str = concat(allocator, foo, cempty);
        REQUIRE(!str.empty());
        REQUIRE(str.size() == 3);
        REQUIRE(str.capacity() == 3);
        REQUIRE(str.data()[3] == '\0');
        REQUIRE(str == "foo");
    }

    {
        String str = concat(allocator, cfoo, empty);
        REQUIRE(!str.empty());
        REQUIRE(str.size() == 3);
        REQUIRE(str.capacity() == 3);
        REQUIRE(str.data()[3] == '\0');
        REQUIRE(str == "foo");
    }

    {
        String str = concat(allocator, empty, foo);
        REQUIRE(!str.empty());
        REQUIRE(str.size() == 3);
        REQUIRE(str.capacity() == 3);
        REQUIRE(str.data()[3] == '\0');
        REQUIRE(str == "foo");
    }

    {
        String str = concat(allocator, cempty, foo);
        REQUIRE(!str.empty());
        REQUIRE(str.size() == 3);
        REQUIRE(str.capacity() == 3);
        REQUIRE(str.data()[3] == '\0');
        REQUIRE(str == "foo");
    }

    {
        String str = concat(allocator, empty, cfoo);
        REQUIRE(!str.empty());
        REQUIRE(str.size() == 3);
        REQUIRE(str.capacity() == 3);
        REQUIRE(str.data()[3] == '\0');
        REQUIRE(str == "foo");
    }
}
