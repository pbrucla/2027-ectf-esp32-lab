#include "test.hpp"
#include "uart.hpp"

static const int a = 1, b = 2;

TEST_CASE(test_a) {
    TEST_ASSERT(a == 1);
    TEST_PASS();
}

TEST_CASE(test_b) {
    TEST_ASSERT(b == 2);
    TEST_PASS();
}

int main() {
    // run a bunch of tests defined above
}
/// TODO: please write unit tests for integrity with hashes