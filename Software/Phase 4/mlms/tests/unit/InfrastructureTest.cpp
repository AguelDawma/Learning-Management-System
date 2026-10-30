#include "TestSupport.hpp"
#include "infrastructure/Sha256.hpp"

TEST(Sha256, KnownVectors) {
    EXPECT_EQ(sha256Hex(""), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    EXPECT_EQ(sha256Hex("abc"), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    EXPECT_EQ(sha256Hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
              "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    EXPECT_EQ(sha256Hex(std::string(1000000, 'a')),
              "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
}

TEST(Sha256, FileMatchesString) {
    TempDir dir;
    EXPECT_EQ(sha256File(dir.write("x.txt", "abc")), sha256Hex("abc"));
}

TEST(Argon2Hasher, SaltsEveryHashAndVerifies) {
    const Argon2Hasher hasher(1, 1024, 1); // small cost to keep the test fast
    const auto h1 = hasher.hash("Password1");
    const auto h2 = hasher.hash("Password1");
    EXPECT_EQ(h1.rfind("$argon2id$", 0), 0u);
    EXPECT_NE(h1, h2); // random salt
    EXPECT_EQ(h1.find("Password1"), std::string::npos);
    EXPECT_TRUE(hasher.verify("Password1", h1));
    EXPECT_FALSE(hasher.verify("password1", h1));
    EXPECT_FALSE(hasher.verify("Password1", "not a hash"));
}

TEST(LocalFileStore, ValidatesTypeSizeAndExistence) {
    TempDir root, src;
    LocalFileStore store(root.path(), 10, {".pdf", ".txt"});
    EXPECT_THROW(store.validate(src.path() / "missing.pdf"), ValidationError);
    EXPECT_THROW(store.validate(src.write("empty.pdf", "")), ValidationError);
    EXPECT_THROW(store.validate(src.write("big.pdf", "01234567890")), ValidationError); // 11 bytes > 10
    EXPECT_NO_THROW(store.validate(src.write("ok.pdf", "0123456789")));               // exactly 10
    EXPECT_NO_THROW(store.validate(src.write("UPPER.PDF", "abc")));
    EXPECT_THROW(store.validate(src.write("virus.exe", "abc")), ValidationError);
    EXPECT_THROW(store.validate(src.path()), ValidationError); // a directory
}

TEST(LocalFileStore, StoresVersionedCopyWithChecksum) {
    TempDir root, src;
    LocalFileStore store(root.path());
    const auto stored = store.store(src.write("my essay.txt", "abc"), 4, 9, 2);
    EXPECT_EQ(stored.relativePath, "a4/s9/v2_my_essay.txt");
    EXPECT_EQ(stored.checksum, sha256Hex("abc"));
    EXPECT_TRUE(std::filesystem::exists(store.resolve(stored.relativePath)));
    // An existing version is never overwritten.
    EXPECT_THROW(store.store(src.write("my essay.txt", "other"), 4, 9, 2), FileStoreError);
    store.remove(stored.relativePath);
    EXPECT_FALSE(std::filesystem::exists(store.resolve(stored.relativePath)));
}
