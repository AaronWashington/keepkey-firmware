extern "C" {
#include "keepkey/crypto/rand.h"
}

#include "gtest/gtest.h"

TEST(Crypto, PermuteChar) {
    char arr[4];
    for (int i = 0; i < sizeof(arr); i++) {
        arr[i] = i;
    }
    random_permute_char(arr, sizeof(arr));
    char count[sizeof(arr)];
    memset(count, 0, sizeof(count));
    for (int i = 0; i < sizeof(arr); i++) {
        count[arr[i]]++;
    }
    for (int i = 0; i < sizeof(arr); i++) {
        ASSERT_EQ(count[i], (char)1);
    }
}

TEST(Crypto, PermuteU16) {
    uint16_t arr[4];
    for (int i = 0; i < sizeof(arr)/sizeof(arr[0]); i++) {
        arr[i] = i;
    }
    random_permute_u16(arr, sizeof(arr)/sizeof(arr[0]));
    uint16_t count[sizeof(arr)/sizeof(arr[0])];
    memset(count, 0, sizeof(count));
    for (int i = 0; i < sizeof(arr)/sizeof(arr[0]); i++) {
        count[arr[i]]++;
    }
    for (int i = 0; i < sizeof(arr)/sizeof(arr[0]); i++) {
        ASSERT_EQ(count[i], 1u);
    }
}
