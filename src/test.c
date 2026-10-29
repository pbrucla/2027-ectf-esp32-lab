#include "test.h"
#include "uart.h"

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
