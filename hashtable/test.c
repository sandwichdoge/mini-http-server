#include <stdio.h>
#include <assert.h>
#include <math.h>

static size_t round_up_power_of_two(size_t n)
{
        size_t p = 0;
        for (int i = 0; p < (size_t)pow(2, 63); ++i) {
                if (n <= p) {
                        break;
                }
                else {
                        p = (size_t)pow(2, i);
                }
        }

        return p;
}

static size_t pseudo_hash(size_t number, size_t max)
{
    return number & (max -1);
}

void test1()
{
    int n1022 = 1022;
    size_t final1022 = round_up_power_of_two(n1022);
    printf("Rounded %d up to %d\n", n1022, final1022);
    assert(final1022 == 1024);
}

void test2()
{
    size_t ret1023 = pseudo_hash(1023, 1024);
    assert(ret1023 == 1023);

    size_t ret1025 = pseudo_hash(1025, 1024);
    assert(ret1025 == 1);
    printf("%d\n", ret1025);

    size_t ret1024 = pseudo_hash(1024, 1024);
    assert(ret1024 == 0);

    size_t ret0 = pseudo_hash(0, 1024);
    assert(ret0 == 0);
}

int main()
{
    test1();
    test2();

    return 0;
}