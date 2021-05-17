// RUN: %check_clang_tidy %s misc-fmt-argument-mismatch %t

#include <fmt/format.h>

void foo() {

    fmt::print("{1} {0}", 0, 1);  // ok

    fmt::print("{}, {}!", "Hello", "world"); // ok

    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: no argument for position 3 in format string [misc-fmt-argument-mismatch]
    fmt::print("{0} {1} {2} {3}", 0, 1, 2);

    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: no argument for position 3 in format string [misc-fmt-argument-mismatch]
    // CHECK-MESSAGES: :[[@LINE-1]]:47: warning: unused argument [misc-fmt-argument-mismatch]
    fmt::print(stdout, "{1} {0} {3}", 4., 3., 2.);

    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: argument count mismatch (expected 4, got 3) [misc-fmt-argument-mismatch]
    fmt::print("{} {} {} {}", "he", "he", "he");

    // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: argument count mismatch (expected 3, got 4) [misc-fmt-argument-mismatch]
    fmt::format("{} {} {}", 0, 1, 2, 3);

     // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: combining of manual and automatic argument indexing in format string restricted [misc-fmt-argument-mismatch]
    fmt::formatted_size("{2} {} {0}", 0, 1, 2);

    std::string buf;

    // CHECK-MESSAGES: :[[@LINE-1]]:45: warning: invalid format string: expected argument id or ':' at character 5 [misc-fmt-argument-mismatch]
    fmt::format_to(std::back_inserter(buf), "{0} {_} ", "foo", "bar");

    fmt::print("{} {} {} {{ }}", 0, 1, 2); // ok
}
