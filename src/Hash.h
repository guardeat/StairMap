#ifndef HASH_H
#define HASH_H

#include <cstdint>
#include <string>

namespace ByteA
{

    template<typename T>
    struct Hash
    {
        constexpr size_t operator()(const T& arg) const
        {
            size_t hash{ static_cast<size_t>(reinterpret_cast<std::uintptr_t>(&arg)) };
            hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
            hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
            hash = ((hash >> 16) ^ hash);
            return hash;
        }
    };

    template<>
    struct Hash<int8_t>
    {
        constexpr uint8_t operator()(int8_t arg) const
        {
            return static_cast<uint8_t>(arg);
        }
    };

    template<>
    struct Hash<int16_t>
    {
        constexpr uint16_t operator()(int16_t arg) const
        {
            return static_cast<uint16_t>(arg);
        }
    };

    template<>
    struct Hash<int32_t>
    {
        constexpr uint32_t operator()(int32_t arg) const
        {
            return static_cast<uint32_t>(arg);
        }
    };

    template<>
    struct Hash<int64_t>
    {
        constexpr uint64_t operator()(int64_t arg) const
        {
            return static_cast<uint64_t>(arg);
        }
    };

    template<>
    struct Hash<uint8_t>
    {
        constexpr uint8_t operator()(uint8_t arg) const
        {
            return arg;
        }
    };

    template<>
    struct Hash<uint16_t>
    {
        constexpr uint16_t operator()(uint16_t arg) const
        {
            return arg;
        }
    };

    template<>
    struct Hash<uint32_t>
    {
        constexpr uint32_t operator()(uint32_t arg) const
        {
            return arg;
        }
    };

    template<>
    struct Hash<uint64_t>
    {
        constexpr uint64_t operator()(uint64_t arg) const
        {
            return arg;
        }
    };

    template<>
    struct Hash<std::string>: public std::hash<std::string>
    {
    };
}

#endif