#include "BudgetBitesLib/PasswordSecurity.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <random>
#include <sstream>
#include <vector>

// =========================================================================
// Minimal self-contained SHA-256 implementation (FIPS 180-4).
// Kept private to this translation unit -- callers only ever see the
// hex digest via PasswordSecurity::hash. This is the "how"; everything
// outside this file only ever deals with the "what" (hash, verify).
//
// Variable names below spell out their role instead of using the single
// letters (a-h, s0/s1, w) that FIPS 180-4 itself uses, so the code reads
// without needing the spec open next to it.
// =========================================================================
namespace {

// The 64 constant words used in the compression function, one per round.
// Each is the fractional part of the cube root of the first 64 primes,
// as fixed by the SHA-256 standard.
constexpr std::array<uint32_t, 64> kRoundConstants = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// Circular right-shift of a 32-bit word: bits pushed off the low end
// wrap around to the high end. SHA-256 uses this (never a plain shift)
// so no information from the input is ever discarded.
inline uint32_t rotateRight(uint32_t value, uint32_t bitCount) {
    return (value >> bitCount) | (value << (32 - bitCount));
}

std::array<uint8_t, 32> sha256Digest(const std::string& message) {
    // The eight initial hash values (H0-H7 in the spec): fractional parts
    // of the square roots of the first 8 primes. These accumulate the
    // running hash across every 64-byte chunk processed below.
    uint32_t hashValue0 = 0x6a09e667, hashValue1 = 0xbb67ae85, hashValue2 = 0x3c6ef372, hashValue3 = 0xa54ff53a;
    uint32_t hashValue4 = 0x510e527f, hashValue5 = 0x9b05688c, hashValue6 = 0x1f83d9ab, hashValue7 = 0x5be0cd19;

    // Copy the message so it can be padded in place to a multiple of the
    // 64-byte block size that SHA-256 processes at a time.
    std::vector<uint8_t> paddedMessage(message.begin(), message.end());
    const uint64_t originalBitLength = static_cast<uint64_t>(paddedMessage.size()) * 8;

    // Padding: a single 1-bit (encoded as the byte 0x80), then zero bytes
    // until the length is 56 mod 64, leaving exactly 8 bytes for the
    // original bit-length that gets appended next.
    paddedMessage.push_back(0x80);
    while (paddedMessage.size() % 64 != 56) {
        paddedMessage.push_back(0x00);
    }
    for (int byteIndex = 7; byteIndex >= 0; --byteIndex) {
        paddedMessage.push_back(static_cast<uint8_t>((originalBitLength >> (byteIndex * 8)) & 0xFF));
    }

    // Process the message one 64-byte chunk at a time.
    for (std::size_t chunkStart = 0; chunkStart < paddedMessage.size(); chunkStart += 64) {
        // The message schedule: 64 words derived from the current chunk.
        // The first 16 are the chunk's bytes taken 4-at-a-time (big-endian);
        // the remaining 48 are expanded from earlier words below.
        std::array<uint32_t, 64> messageSchedule{};
        for (int wordIndex = 0; wordIndex < 16; ++wordIndex) {
            std::size_t byteOffset = chunkStart + wordIndex * 4;
            messageSchedule[wordIndex] = (static_cast<uint32_t>(paddedMessage[byteOffset]) << 24) |
                                         (static_cast<uint32_t>(paddedMessage[byteOffset + 1]) << 16) |
                                         (static_cast<uint32_t>(paddedMessage[byteOffset + 2]) << 8) |
                                         (static_cast<uint32_t>(paddedMessage[byteOffset + 3]));
        }
        for (int wordIndex = 16; wordIndex < 64; ++wordIndex) {
            // Mixing functions (little sigma 0 / little sigma 1) that
            // spread each new schedule word's bits across earlier words,
            // so a one-bit change in the input affects the whole digest.
            uint32_t messageScheduleSigma0 = rotateRight(messageSchedule[wordIndex - 15], 7) ^
                                              rotateRight(messageSchedule[wordIndex - 15], 18) ^
                                              (messageSchedule[wordIndex - 15] >> 3);
            uint32_t messageScheduleSigma1 = rotateRight(messageSchedule[wordIndex - 2], 17) ^
                                              rotateRight(messageSchedule[wordIndex - 2], 19) ^
                                              (messageSchedule[wordIndex - 2] >> 10);
            messageSchedule[wordIndex] = messageSchedule[wordIndex - 16] + messageScheduleSigma0 +
                                         messageSchedule[wordIndex - 7] + messageScheduleSigma1;
        }

        // Eight working variables, seeded from the running hash values
        // and then updated once per round below.
        uint32_t workingA = hashValue0, workingB = hashValue1, workingC = hashValue2, workingD = hashValue3;
        uint32_t workingE = hashValue4, workingF = hashValue5, workingG = hashValue6, workingH = hashValue7;

        for (int roundIndex = 0; roundIndex < 64; ++roundIndex) {
            // Big sigma 1 of E, and the "choose" function: where E's bits
            // are 1, pick F's bit; where they're 0, pick G's bit instead.
            uint32_t bigSigma1OfE = rotateRight(workingE, 6) ^ rotateRight(workingE, 11) ^ rotateRight(workingE, 25);
            uint32_t chooseFunction = (workingE & workingF) ^ ((~workingE) & workingG);
            uint32_t roundTemp1 = workingH + bigSigma1OfE + chooseFunction + kRoundConstants[roundIndex] + messageSchedule[roundIndex];

            // Big sigma 0 of A, and the "majority" function: each output
            // bit is whichever value (0 or 1) appears in at least two of
            // A, B, C at that bit position.
            uint32_t bigSigma0OfA = rotateRight(workingA, 2) ^ rotateRight(workingA, 13) ^ rotateRight(workingA, 22);
            uint32_t majorityFunction = (workingA & workingB) ^ (workingA & workingC) ^ (workingB & workingC);
            uint32_t roundTemp2 = bigSigma0OfA + majorityFunction;

            // Shift the working variables down one slot, folding in the
            // two temporaries computed above.
            workingH = workingG;
            workingG = workingF;
            workingF = workingE;
            workingE = workingD + roundTemp1;
            workingD = workingC;
            workingC = workingB;
            workingB = workingA;
            workingA = roundTemp1 + roundTemp2;
        }

        // Fold this chunk's result into the running hash values.
        hashValue0 += workingA; hashValue1 += workingB; hashValue2 += workingC; hashValue3 += workingD;
        hashValue4 += workingE; hashValue5 += workingF; hashValue6 += workingG; hashValue7 += workingH;
    }

    // Serialize the eight 32-bit hash values into 32 bytes, big-endian.
    std::array<uint8_t, 32> digestBytes{};
    uint32_t finalHashValues[8] = {hashValue0, hashValue1, hashValue2, hashValue3,
                                   hashValue4, hashValue5, hashValue6, hashValue7};
    for (int hashWordIndex = 0; hashWordIndex < 8; ++hashWordIndex) {
        digestBytes[hashWordIndex * 4]     = static_cast<uint8_t>((finalHashValues[hashWordIndex] >> 24) & 0xFF);
        digestBytes[hashWordIndex * 4 + 1] = static_cast<uint8_t>((finalHashValues[hashWordIndex] >> 16) & 0xFF);
        digestBytes[hashWordIndex * 4 + 2] = static_cast<uint8_t>((finalHashValues[hashWordIndex] >> 8) & 0xFF);
        digestBytes[hashWordIndex * 4 + 3] = static_cast<uint8_t>(finalHashValues[hashWordIndex] & 0xFF);
    }
    return digestBytes;
}

// A single templated hex-encoder used for both std::array and std::vector
// byte buffers, rather than writing the same loop out twice.
// (Avoids the "Duplicated Code" smell -- Fowler.)
template <typename ByteContainer>
std::string toHex(const ByteContainer& byteContainer) {
    std::ostringstream hexStream;
    hexStream << std::hex << std::setfill('0');
    for (unsigned char byteValue : byteContainer) {
        hexStream << std::setw(2) << static_cast<int>(byteValue);
    }
    return hexStream.str();
}

// Compares two strings in time proportional only to their length, never
// short-circuiting on the first mismatch. A naive == would return faster
// for a hash that mismatches earlier, letting an attacker learn correct
// hash bytes one at a time by timing failed sign-in attempts.
bool constantTimeEquals(const std::string& computedHash, const std::string& expectedHash) {
    if (computedHash.size() != expectedHash.size()) {
        return false;
    }
    unsigned char mismatchAccumulator = 0;
    for (std::size_t byteIndex = 0; byteIndex < computedHash.size(); ++byteIndex) {
        mismatchAccumulator |= static_cast<unsigned char>(computedHash[byteIndex]) ^
                                static_cast<unsigned char>(expectedHash[byteIndex]);
    }
    return mismatchAccumulator == 0;
}

} // namespace

namespace PasswordSecurity {

bool isStrong(const std::string& password) {
    if (password.size() < kMinPasswordLength) {
        return false;
    }
    bool hasUpper = false, hasLower = false, hasDigit = false, hasSpecial = false;
    for (unsigned char currentChar : password) {
        if (std::isupper(currentChar)) hasUpper = true;
        else if (std::islower(currentChar)) hasLower = true;
        else if (std::isdigit(currentChar)) hasDigit = true;
        else if (std::ispunct(currentChar)) hasSpecial = true;
    }
    return hasUpper && hasLower && hasDigit && hasSpecial;
}

std::string generateSalt(std::size_t numBytes) {
    // std::random_device is the OS-provided source of non-deterministic
    // randomness; a salt must be unpredictable, so a seeded PRNG (like
    // std::mt19937) would defeat the purpose here.
    std::random_device randomDevice;
    std::uniform_int_distribution<int> byteDistribution(0, 255);
    std::vector<uint8_t> saltBytes(numBytes);
    for (auto& randomByte : saltBytes) {
        randomByte = static_cast<uint8_t>(byteDistribution(randomDevice));
    }
    return toHex(saltBytes);
}

std::string hash(const std::string& password, const std::string& salt) {
    return toHex(sha256Digest(password + salt));
}

bool verify(const std::string& password, const std::string& salt, const std::string& expectedHash) {
    return constantTimeEquals(hash(password, salt), expectedHash);
}

} // namespace PasswordSecurity