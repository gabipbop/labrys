#pragma once

#include <array>
#include <cstdint>

namespace dedale
{
// xoshiro128** : petit générateur rapide et déterministe (seed fixe dans les tests, aléatoire en usage
// normal).
class Rng
{
public:
    explicit Rng(std::uint32_t seedValue = 1) noexcept { seed(seedValue); }

    void seed(std::uint32_t seedValue) noexcept
    {
        // splitmix32 : étale la seed sur les 4 mots d'état (qui ne doivent pas être tous nuls).
        for (auto& word : state)
        {
            seedValue += 0x9e3779b9u;
            auto z = seedValue;
            z = (z ^ (z >> 16)) * 0x85ebca6bu;
            z = (z ^ (z >> 13)) * 0xc2b2ae35u;
            word = z ^ (z >> 16);
        }
        state[0] |= 1u;
    }

    std::uint32_t next() noexcept
    {
        const auto result = rotl(state[1] * 5u, 7) * 9u;
        const auto t = state[1] << 9;
        state[2] ^= state[0];
        state[3] ^= state[1];
        state[1] ^= state[2];
        state[0] ^= state[3];
        state[2] ^= t;
        state[3] = rotl(state[3], 11);
        return result;
    }

    // Uniforme dans [0, 1).
    float uniform() noexcept { return static_cast<float>(next() >> 8) * (1.0f / 16777216.0f); }

    // Uniforme dans [-5, +5) V.
    float bipolarVolts() noexcept { return uniform() * 10.0f - 5.0f; }

private:
    static std::uint32_t rotl(std::uint32_t x, int k) noexcept { return (x << k) | (x >> (32 - k)); }

    std::array<std::uint32_t, 4> state{};
};
} // namespace dedale
