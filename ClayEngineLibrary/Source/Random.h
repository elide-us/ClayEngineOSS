#pragma once

#include <random>
#include <numeric>
#include <limits>

namespace ClayEngine
{
    constexpr auto c_random_seed = 1974ULL;
    static std::mt19937 s_rand(c_random_seed);

    inline static int GetNextInt(int min, int max)
    {
        auto d = std::uniform_int_distribution<int>(min, max);
        return d(s_rand);
    }

    inline static int GetNextInt()
    {
        static std::uniform_int_distribution<int> s_dist(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        return s_dist(s_rand);
    }

    inline static float GetNextFloat()
    {
        auto d = std::uniform_real_distribution<float>(0.f, 1.f);
        return d(s_rand);
    }

    inline static int GetNext3D6()
    {
        auto d = std::uniform_int_distribution<int>(1, 6);
        return d(s_rand) + d(s_rand) + d(s_rand);
    }

    inline static int GetNextNDX(int number, int faces)
    {
        auto d = std::uniform_int_distribution<int>(1, faces);

        std::vector<int> v{};
        for (auto it = 0; it < number; ++it)
        {
            v.push_back(d(s_rand));
        }

        return std::accumulate(v.begin(), v.end(), 0);
    }
}

//inline bool GetBitAt(char packed, int index)
//{
//    switch (index)
//    {
//    case 8:
//        return packed & 0x01;
//    case 7:
//        return (packed & 0x02) >> 1;
//    case 6:
//        return (packed & 0x04) >> 2;
//    case 5:
//        return (packed & 0x08) >> 3;
//    case 4:
//        return (packed & 0x10) >> 4;
//    case 3:
//        return (packed & 0x20) >> 5;
//    case 2:
//        return (packed & 0x40) >> 6;
//    case 1:
//        return (packed & 0x80) >> 7;
//    default:
//        throw;
//    }
//}

//template<typename... T>
//auto FoldSum(T... s)
//{
//    return (... + s);
//}
