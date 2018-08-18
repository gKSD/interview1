#include <iostream>
#include <string>
#include "assert.h"

std::size_t search_str(const std::string &str, const std::string &wildcard, std::size_t pos = 0)
{
    bool asterisk = false;
    int question = 0;

    std::size_t res_pos = pos;
    std::size_t after_last_matched_symbol = pos;  // last symbol in str matched some symbol from wildcard
    int i = 0;
    for (; i < wildcard.size();)
    {
        if (pos == str.size())
        {
            while (wildcard[i] == '*' && i < wildcard.size()) ++i;
            if (i < wildcard.size()
                || (pos - after_last_matched_symbol) < question )
            {
                return std::string::npos;
            }

            break;
        }

        switch (wildcard[i])
        {
            case '*':
            {
                asterisk = true;

                ++i;
                continue;
            }
            case '?':
            {
                ++question;
                if (pos + question > str.size())
                {
                    return std::string::npos;
                }

                ++i;
                continue;
            }
        };

        // wildcard[i] - is some symbol, not '*' or '?'

        if (asterisk)
        {
            std::size_t fnd = str.find(wildcard[i], pos);
            if (fnd == std::string::npos)
            {
                return std::string::npos;
            }
            pos = fnd;
        }
        else if (question > 0)
        {
            --question;
            ++pos;
            continue;
        }
        else if (str[pos] != wildcard[i])
        {
            // substr don't match, start matching from current
            pos = after_last_matched_symbol = ++res_pos;
            i = 0;
            asterisk = false;
            question = 0;
            continue;
        }

        if (question > 0 && (pos - after_last_matched_symbol + 1) < question)
        {
            // substr don't match, start matching from current
            pos = after_last_matched_symbol = ++res_pos;
            i = 0;
            asterisk = false;
            question = 0;
            continue;
        }

        ++pos;
        ++i;

        after_last_matched_symbol = pos;
        asterisk = false;
        question = 0;
    }

    return res_pos;
}

void run_test(const std::string &text, const std::string &pattern, std::size_t expected)
{
    auto res = search_str(text, pattern);
    std::cout << "TEST: '" << text << "',\tPATTERN: '" << pattern << "'";
    if (res == std::string::npos)
        std::cout << ",\tRESULT: " << "npos";
    else
        std::cout << ",\tRESULT: " << res;

    std::cout << ",\tEXPECTED: " << expected;
    std::cout << std::endl;

    assert(res == expected);
}

int main(int argc, char **argv)
{
    run_test("abc", ".*", std::string::npos);

    run_test("", "*", 0);
    run_test("", "?*", std::string::npos);
    run_test("a", "?*", 0);

    run_test("a", "*????*", std::string::npos);
    run_test("aaaa", "*????*", 0);
    run_test("aaaaa", "*????*", 0);

    run_test("abac", "*", 0);
    run_test("abac", "", 0);
    run_test("", "", 0);
    run_test("", "*", 0);
    run_test("", "?", std::string::npos);
    run_test("", "a", std::string::npos);
    run_test("", "a?", std::string::npos);
    run_test("", "a*", std::string::npos);
    run_test("aa", "?", 0);

    run_test("aaabbbccc", "aaa", 0);
    run_test("aaabbbccc", "bbb", 3);
    run_test("aaabbbccc", "ccc", 6);

    run_test("abcdef", "b*d", 1);
    run_test("abcdef", "*def", 0);
    run_test("abcdef", "def*", 3);
    run_test("abcdef", "cde*", 2);

    run_test("abcdef", "?de", 2);
    run_test("cdef", "?de", 0);
    run_test("cdef", "de?", 1);
    run_test("cdef", "def?", std::string::npos);

    run_test("cdef", "def*****", 1);
    run_test("cdef", "****cdef", 0);
    run_test("cdef", "****def", 0);
    run_test("cdef", "****", 0);
    run_test("cdef", "*", 0);
    run_test("cdef", "cdef***", 0);
    run_test("cdef", "cde****", 0);
    run_test("cdef", "*cdef*", 0);

    run_test("cdef", "def????", std::string::npos);
    run_test("cdef", "????", 0);
    run_test("cdef", "?cdef", std::string::npos);
    run_test("cdef", "cdef?", std::string::npos);
    run_test("cdef", "?cdef?", std::string::npos);
    run_test("cdef", "?cde?", std::string::npos);
    run_test("cdef", "?de?", 0);
    run_test("cdef", "c??f", 0);

    run_test("cdef", "?***", 0);
    run_test("cdef", "***?", 0);
    run_test("cdef", "***?***", 0);
    run_test("cdef", "*?*?*?*?", 0);
    run_test("cdef", "?*?*?*?*", 0);
    run_test("cdef", "*?", 0);
    run_test("cdef", "c*?", 0);
    run_test("cdef", "?*f", 0);
    run_test("cdef", "?*e", 0);

    return 0;
}
