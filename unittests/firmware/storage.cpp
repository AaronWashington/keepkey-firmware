extern "C" {
#include "keepkey/firmware/storage.h"
#include "keepkey/firmware/policy.h"
#include "keepkey/board/keepkey_board.h"
#include "types.pb.h"
#include "storage.h"
}

#include "gtest/gtest.h"

#include <cstring>
#include <string>

TEST(Storage, ReadMeta) {
    Metadata dst;
    const char src[] = "M1M2u1u2u3u4u5u6S1S2S3S4S5S6S7S8S9SASBSC";

    storage_readMeta(&dst, src);

    ASSERT_TRUE(memcmp(dst.magic, "M1M2", 4) == 0);
    ASSERT_TRUE(memcmp(dst.uuid, "u1u2u3u4u5u6", 12) == 0);
    ASSERT_TRUE(memcmp(dst.uuid_str, "S1S2S3S4S5S6S7S8S9SASBSC", 24) == 0);

    for (int i = 0; i < sizeof(src); i++) {
        ASSERT_EQ(src[i], ((char *)&dst)[i]) << "i: " << i;
    }
}

TEST(Storage, WriteMeta) {
    const Metadata src = {
        .magic = "M1M",
        .uuid = "u1u2u3u4u5u",
        .uuid_str = "S1S2S3S4S5S6S7S8S9SASBS",
    };

    char dst[41];
    memset(dst, 0, sizeof(dst));

    storage_writeMeta(&dst[0], &src);

    ASSERT_TRUE(memcmp(dst, "M1M\0u1u2u3u4u5u\0S1S2S3S4S5S6S7S8S9SASBS\0", sizeof(dst)) == 0);
}

TEST(Storage, ReadPolicy) {
    PolicyType dst;
    const char src[] = "\x01N1N2N3N4N5N6N7N\x01\x01";

    storage_readPolicy(&dst, src);

    ASSERT_EQ(dst.has_policy_name, true);
    ASSERT_TRUE(memcmp(dst.policy_name, "N1N2N3N4N5N6N7N8N", 15) == 0);
    ASSERT_EQ(dst.has_enabled, true);
    ASSERT_EQ(dst.enabled, true);
}

TEST(Storage, WritePolicy) {
    const PolicyType src = {
        .has_policy_name = true,
        .policy_name = "0123456789ABCD",
        .has_enabled = true,
        .enabled = true,
    };

    char dst[18];
    memset(dst, 0, sizeof(dst));

    storage_writePolicy(&dst[0], &src);

    ASSERT_TRUE(memcmp(dst, "\x01""0123456789ABCD\0\x01\x01", sizeof(dst)) == 0);
}

TEST(Storage, ReadStorageV1) {
    const char src[483] =
        /*   0 */ "\x0a\x00\x00\x00" // version
        /*   4 */ "\x01"             // has_node
                  "\x00\x00\x00"     // reserved
        /*   8 */ "\x03\x00\x00\x00" // depth
        /*  12 */ "\x2a\x00\x00\x00" // fingerprint
        /*  16 */ "\x11\x00\x00\x00" // child_num
        /*  20 */ "\x20\x00\x00\x00" // chain_code.size
        /*  24 */ "\x58\x4d\x52\x58\x4d\x52\x58\x4d\x52\x58\x4d\x52\x58\x4d\x52\x58" // chain_code.bytes
                  "\x4d\x52\x58\x4d\x52\x58\x4d\x52\x58\x4d\x52\x58\x4d\x52\x58\x00" // chain_code.bytes
        /*  56 */ "\x01"             // has_private_key
                  "\x00\x00\x00"     // reserved
        /*  60 */ "\x20\x00\x00\x00" // private_key.size
        /*  64 */ "\x46\x4f\x58\x59\x4b\x50\x4b\x59\x46\x4f\x58\x59\x4b\x50\x4b\x59" // private_key.bytes
                  "\x46\x4f\x58\x59\x4b\x50\x4b\x59\x46\x4f\x58\x59\x4b\x50\x4b\x00" // private_key.bytes
        /*  96 */ "\x01"             // has_public_key
                  "\x00\x00\x00"     // reserved
        /* 100 */ "\x21\x00\x00\x00" // public_key.size
        /* 104 */ "\x57\x68\x6f\x20\x69\x73\x20\x53\x61\x74\x6f\x73\x68\x69\x20\x4e" // public_key.bytes
                  "\x61\x6b\x6f\x6d\x6f\x74\x6f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f" // public_key.bytes
        /* 136 */ "\x00"             // public_key.bytes
        /* 137 */ "\x00\x00\x00"     // reserved
        /* 140 */ "\x01"             // has_mnemonic
        /* 141 */ "\x7a\x6f\x6f\x20\x7a\x6f\x6f\x20\x7a\x6f\x6f\x20\x7a\x6f\x6f\x20" // mnemonic
                  "\x7a\x6f\x6f\x20\x7a\x6f\x6f\x20\x7a\x6f\x6f\x20\x7a\x6f\x6f\x20" // mnemonic
                  "\x7a\x6f\x6f\x20\x7a\x6f\x6f\x20\x7a\x6f\x6f\x20\x77\x72\x6f\x6e" // menmonic
                  "\x67\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00"             // mnemonic
        /* 382 */ "\x00"             // has_passphrase_protection
        /* 383 */ "\x00"             // passphrase_protection
        /* 384 */ "\x01"             // has_pin_failed_attempts
                  "\x00\x00\x00"     // reserved
        /* 388 */ "\x2a\x00\x00\x00" // pin_failed_attempts
        /* 392 */ "\x01"             // has_pin
        /* 393 */ "\x31\x32\x33\x34\x35\x36\x37\x38\x39\x00" // pin
        /* 403 */ "\x01"             // has_language
        /* 404 */ "\x65\x73\x70\x65\x72\x61\x6e\x74\x6f\x00\x00\x00\x00\x00\x00\x00" // language
                  "\x00"             // language
        /* 421 */ "\x01"             // has_label
        /* 422 */ "\x4d\x65\x6e\x6f\x73\x4d\x61\x72\x78\x4d\x61\x69\x73\x4d\x69\x73" // label
                  "\x65\x73\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // label
                  "\x00"             // label
        /* 455 */ "\x01"             // has_imported
        /* 456 */ "\x00"             // imported
        /* 457 */ "\x00\x00\x00"     // reserved
        /* 460 */ "\x01\x00\x00\x00" // policies_count
        /* 464 */ "\x01"             // policies[0].has_policy_name
        /* 465 */ "\x53\x68\x61\x70\x65\x53\x68\x69\x66\x74\x00\x00\x00\x00\x00" // policies[0].policy_name
        /* 481 */ "\x01"             // policies[0].has_enabled
        /* 482 */ "\x01";            // policies[0].enabled

    Storage dst;
    memset(&dst, 0xCC, sizeof(dst));

    storage_readStorageV1(&dst, &src[0]);

    // Check a few, don't need toc check them all.
    EXPECT_EQ(dst.version, 10);
    EXPECT_TRUE(dst.has_node);
    EXPECT_EQ(dst.node.depth, 3);
    EXPECT_EQ(dst.node.fingerprint, 42);
    EXPECT_EQ(dst.node.child_num, 17);
    EXPECT_EQ(dst.node.chain_code.size, 32);
    EXPECT_TRUE(memcmp(dst.node.chain_code.bytes, "XMRXMRXMRXMRXMRXMRXMRXMRXMRXMRX\0", 32) == 0);
    EXPECT_EQ(dst.node.has_private_key, true);
    EXPECT_EQ(dst.node.public_key.size, 33);
    EXPECT_TRUE(memcmp(dst.node.public_key.bytes, "Who is Satoshi Nakomoto?????????\0", 32) == 0);
    EXPECT_EQ(dst.has_mnemonic, true);

    char dst2[482];
    memset(dst2, 0xD0, sizeof(dst2));
    storage_writeStorageV1(&dst2[0], &dst);

    for (int i = 0; i < sizeof(dst2); ++i) {
        ASSERT_EQ(src[i], dst2[i]) << "i: " << i;
    }

    memset(&dst, 0x00, sizeof(dst));
    storage_readStorageV1(&dst, &src[0]);

    for (int i = 0; i < sizeof(dst) - 1; i++) {
        ASSERT_EQ(src[i], ((char*)&dst)[i]) << "i: " << i;
    }
}

TEST(Storage, WriteStorageV1) {
    const Storage src = {
        .version = 10,
        .has_node = true,
        .node = {
            .depth = 3,
            .fingerprint = 42,
            .child_num = 17,
            .chain_code = {
                .size = 32,
                .bytes = "XMRXMRXMRXMRXMRXMRXMRXMRXMRXMRX",
            },
            .has_private_key = true,
            .private_key = {
                .size = 32,
                .bytes = "FOXYKPKYFOXYKPKYFOXYKPKYFOXYKPK",
            },
            .has_public_key = true,
            .public_key = {
                .size = 33,
                .bytes = "Who is Satoshi Nakomoto?????????",
            },
        },
        .has_mnemonic = true,
        .mnemonic = "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo wrong",
        .has_passphrase_protection = false,
        .passphrase_protection = false,
        .has_pin_failed_attempts = true,
        .pin_failed_attempts = 42,
        .has_pin = true,
        .pin = "123456789",
        .has_language = true,
        .language = "esperanto",
        .has_label = true,
        .label = "MenosMarxMaisMises",
        .has_imported = true,
        .imported = false,
        .policies_count = 1,
        .policies = {{
            .has_policy_name = true,
            .policy_name = "ShapeShift",
            .has_enabled = true,
            .enabled = true,
        }},
    };

    char dst[482];
    memset(dst, 0xCC, sizeof(dst));

    storage_writeStorageV1(&dst[0], &src);

    const char expected[483] =
        /*   0 */ "\x0a\x00\x00\x00" // version
        /*   4 */ "\x01"             // has_node
                  "\x00\x00\x00"     // reserved
        /*   8 */ "\x03\x00\x00\x00" // depth
        /*  12 */ "\x2a\x00\x00\x00" // fingerprint
        /*  16 */ "\x11\x00\x00\x00" // child_num
        /*  20 */ "\x20\x00\x00\x00" // chain_code.size
        /*  24 */ "\x58\x4d\x52\x58\x4d\x52\x58\x4d\x52\x58\x4d\x52\x58\x4d\x52\x58" // chain_code.bytes
                  "\x4d\x52\x58\x4d\x52\x58\x4d\x52\x58\x4d\x52\x58\x4d\x52\x58\x00" // chain_code.bytes
        /*  56 */ "\x01"             // has_private_key
                  "\x00\x00\x00"     // reserved
        /*  60 */ "\x20\x00\x00\x00" // private_key.size
        /*  64 */ "\x46\x4f\x58\x59\x4b\x50\x4b\x59\x46\x4f\x58\x59\x4b\x50\x4b\x59" // private_key.bytes
                  "\x46\x4f\x58\x59\x4b\x50\x4b\x59\x46\x4f\x58\x59\x4b\x50\x4b\x00" // private_key.bytes
        /*  96 */ "\x01"             // has_public_key
                  "\x00\x00\x00"     // reserved
        /* 100 */ "\x21\x00\x00\x00" // public_key.size
        /* 104 */ "\x57\x68\x6f\x20\x69\x73\x20\x53\x61\x74\x6f\x73\x68\x69\x20\x4e" // public_key.bytes
                  "\x61\x6b\x6f\x6d\x6f\x74\x6f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f" // public_key.bytes
        /* 136 */ "\x00"             // public_key.bytes
        /* 137 */ "\x00\x00\x00"     // reserved
        /* 140 */ "\x01"             // has_mnemonic
        /* 141 */ "\x7a\x6f\x6f\x20\x7a\x6f\x6f\x20\x7a\x6f\x6f\x20\x7a\x6f\x6f\x20" // mnemonic
                  "\x7a\x6f\x6f\x20\x7a\x6f\x6f\x20\x7a\x6f\x6f\x20\x7a\x6f\x6f\x20" // mnemonic
                  "\x7a\x6f\x6f\x20\x7a\x6f\x6f\x20\x7a\x6f\x6f\x20\x77\x72\x6f\x6e" // menmonic
                  "\x67\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // mnemonic
                  "\x00"             // mnemonic
        /* 382 */ "\x00"             // has_passphrase_protection
        /* 383 */ "\x00"             // passphrase_protection
        /* 384 */ "\x01"             // has_pin_failed_attempts
                  "\x00\x00\x00"     // reserved
        /* 388 */ "\x2a\x00\x00\x00" // pin_failed_attempts
        /* 392 */ "\x01"             // has_pin
        /* 393 */ "\x31\x32\x33\x34\x35\x36\x37\x38\x39\x00" // pin
        /* 403 */ "\x01"             // has_language
        /* 404 */ "\x65\x73\x70\x65\x72\x61\x6e\x74\x6f\x00\x00\x00\x00\x00\x00\x00" // language
                  "\x00"             // language
        /* 421 */ "\x01"             // has_label
        /* 422 */ "\x4d\x65\x6e\x6f\x73\x4d\x61\x72\x78\x4d\x61\x69\x73\x4d\x69\x73" // label
                  "\x65\x73\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // label
                  "\x00"             // label
        /* 455 */ "\x01"             // has_imported
        /* 456 */ "\x00"             // imported
        /* 457 */ "\x00\x00\x00"     // reserved
        /* 460 */ "\x01\x00\x00\x00" // policies_count
        /* 464 */ "\x01"             // policies[0].has_policy_name
        /* 465 */ "\x53\x68\x61\x70\x65\x53\x68\x69\x66\x74\x00\x00\x00\x00\x00" // policies[0].policy_name
        /* 481 */ "\x01"             // policies[0].has_enabled
        /* 482 */ "\x01";            // policies[0].enabled

    for (int i = 0; i < sizeof(dst); ++i) {
        ASSERT_EQ(dst[i], expected[i]) << "i: " << i;
    }
}

TEST(Storage, WriteCacheV1) {
    const Cache src = {
        .root_seed_cache_status = 42,
        .root_seed_cache = "012345678901234567890123456789012345678901234567890123456789012",
        .root_ecdsa_curve_type = "secp256k1",
    };

    char dst[75];
    memset(dst, 0xCC, sizeof(dst));

    storage_writeCacheV1(&dst[0], &src);

    const char expected[76] = "\x2a\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x00\x73\x65\x63\x70\x32\x35\x36\x6b\x31";

    for (int i = 0; i < sizeof(dst); i++) {
        ASSERT_EQ(dst[i], expected[i]) << "i: " << i;
        ASSERT_EQ(dst[i], ((const char *)&src)[i]) << "i: " << i;
    }
}

TEST(Storage, ReadCacheV1) {
    const char src[76] = "\x2a\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x00\x73\x65\x63\x70\x32\x35\x36\x6b\x31";

    Cache dst;

    storage_readCacheV1(&dst, &src[0]);

    ASSERT_EQ(dst.root_seed_cache_status, 42);
    for (int i = 0; i < 64; i++) {
        ASSERT_EQ(dst.root_seed_cache[i], "012345678901234567890123456789012345678901234567890123456789012"[i]) << "i: " << i;
    }
    ASSERT_TRUE(memcmp(dst.root_ecdsa_curve_type, "secp256k1", 10) == 0);

    for (int i = 0; i < sizeof(src) - 1; ++i) {
        ASSERT_EQ(src[i], ((char *)&dst)[i]) << "i: " << i;
    }
}

TEST(Storage, DumpNode) {
    HDNodeType dst;
    StorageHDNode src = {
      .depth = 42,
      .fingerprint = 37,
      .child_num = 11,
      .chain_code = { 4, { 1, 2, 3, 4 } },
      .has_private_key = true,
      .private_key = { 3, { 5, 6, 7 } },
      .has_public_key = true,
      .public_key = { 1, { 74 } },
    };

    memset(&dst, 0, sizeof(dst));
    storage_dumpNode(&dst, &src);

#if !DEBUG_LINK
    EXPECT_EQ(dst.depth, 0);
    EXPECT_EQ(dst.fingerprint, 0);
    EXPECT_EQ(dst.child_num, 0);
    EXPECT_EQ(dst.chain_code.size, 0);
    EXPECT_EQ(dst.chain_code.bytes[0], 0);
    EXPECT_EQ(dst.has_private_key, 0);
    EXPECT_EQ(dst.private_key.size, 0);
    EXPECT_EQ(dst.private_key.bytes[0], 0);
    EXPECT_EQ(dst.has_public_key, 0);
    EXPECT_EQ(dst.public_key.size, 0);
    EXPECT_EQ(dst.public_key.bytes[0], 0);
#else
    EXPECT_EQ(dst.depth, src.depth);
    EXPECT_EQ(dst.fingerprint, src.fingerprint);
    EXPECT_EQ(dst.child_num, src.child_num);
    EXPECT_EQ(dst.chain_code.size, src.chain_code.size);
    EXPECT_EQ(dst.chain_code.bytes[0], src.chain_code.bytes[0]);
    EXPECT_EQ(dst.has_private_key, src.has_private_key);
    EXPECT_EQ(dst.private_key.size, src.private_key.size);
    EXPECT_EQ(dst.private_key.bytes[0], src.private_key.bytes[0]);
    EXPECT_EQ(dst.has_public_key, src.has_public_key);
    EXPECT_EQ(dst.public_key.size, src.public_key.size);
    EXPECT_EQ(dst.public_key.bytes[0], src.public_key.bytes[0]);

    memset(&dst, 0, sizeof(dst));
    src.has_private_key = false;
    storage_dumpNode(&dst, &src);

    EXPECT_EQ(dst.depth, src.depth);
    EXPECT_EQ(dst.fingerprint, src.fingerprint);
    EXPECT_EQ(dst.child_num, src.child_num);
    EXPECT_EQ(dst.chain_code.size, src.chain_code.size);
    EXPECT_EQ(dst.chain_code.bytes[0], src.chain_code.bytes[0]);
    EXPECT_EQ(dst.has_private_key, src.has_private_key);
    EXPECT_EQ(dst.private_key.size, 0);
    EXPECT_EQ(dst.private_key.bytes[0], 0);
    EXPECT_EQ(dst.has_public_key, src.has_public_key);
    EXPECT_EQ(dst.public_key.size, src.public_key.size);
    EXPECT_EQ(dst.public_key.bytes[0], src.public_key.bytes[0]);

    memset(&dst, 0, sizeof(dst));
    src.has_public_key = false;
    storage_dumpNode(&dst, &src);

    EXPECT_EQ(dst.depth, src.depth);
    EXPECT_EQ(dst.fingerprint, src.fingerprint);
    EXPECT_EQ(dst.child_num, src.child_num);
    EXPECT_EQ(dst.chain_code.size, src.chain_code.size);
    EXPECT_EQ(dst.chain_code.bytes[0], src.chain_code.bytes[0]);
    EXPECT_EQ(dst.has_private_key, src.has_private_key);
    EXPECT_EQ(dst.private_key.size, 0);
    EXPECT_EQ(dst.private_key.bytes[0], 0);
    EXPECT_EQ(dst.has_public_key, src.has_public_key);
    EXPECT_EQ(dst.public_key.size, 0);
    EXPECT_EQ(dst.public_key.bytes[0], 0);
#endif
}

static void check_policyIsSame(const PolicyType *lhs, const PolicyType *rhs) {
    EXPECT_EQ(lhs->has_policy_name, rhs->has_policy_name);

    if (lhs->has_policy_name) {
        EXPECT_EQ(std::string(lhs->policy_name), std::string(rhs->policy_name));
    }

    EXPECT_EQ(lhs->has_enabled, rhs->has_enabled);
    EXPECT_EQ(lhs->enabled, rhs->enabled);
}

TEST(Storage, ResetPolicies) {
    Storage storage;
    memset(&storage, 0xCC, sizeof(storage));

    storage_resetPolicies(&storage);

    EXPECT_EQ(storage.policies_count, POLICY_COUNT);

    for (int i = 0; i < POLICY_COUNT; ++i) {
        check_policyIsSame(&storage.policies[i], &policies[i]);
    }
}

TEST(Storage, ResetCache) {
    Cache src;
    memset(&src, 0xCC, sizeof(src));

    storage_resetCache(&src);

    char dst[sizeof(src)];
    memset(&dst[0], 0, sizeof(dst));

    EXPECT_EQ(memcmp(&src, &dst, sizeof(Cache)), 0);
}
