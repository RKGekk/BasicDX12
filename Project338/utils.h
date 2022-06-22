#pragma once

#include <exception>

#include <algorithm>
#include <cstdint>
#include <codecvt>
#include <locale>
#include <string>
#include <thread>
#include <iomanip>
#include <sstream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <comdef.h>

#include <d3d12.h>

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#pragma warning( push )
#pragma warning( disable : 4996)

inline std::wstring ConvertString(const std::string& string) {
    static std::locale loc("");
    auto& facet = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(loc);
    return std::wstring_convert<std::remove_reference<decltype(facet)>::type, wchar_t>(&facet).from_bytes(string);
}

inline std::string ConvertString(const std::wstring& wstring) {
    static std::locale loc("");
    auto& facet = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(loc);
    return std::wstring_convert<std::remove_reference<decltype(facet)>::type, wchar_t>(&facet).to_bytes(wstring);
}

inline std::wstring to_wstring(const std::string& s) {
    return ConvertString(s);
}

inline const std::wstring& to_wstring(const std::wstring& s) {
    return s;
}

inline std::wstring to_wstring(char c) {
    return to_wstring(std::string(1, c));
}

#pragma warning( pop )

inline void ThrowIfFailed(HRESULT hr) {
    if (FAILED(hr)) {
        _com_error err(hr);
        OutputDebugString(err.ErrorMessage());
        std::string err_str = ConvertString(err.ErrorMessage());
        throw std::exception(err_str.c_str());
    }
}


const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack( push, 8 )
typedef struct tagTHREADNAME_INFO {
    DWORD  dwType;      // Must be 0x1000.
    LPCSTR szName;      // Pointer to name (in user addr space).
    DWORD  dwThreadID;  // Thread ID (-1=caller thread).
    DWORD  dwFlags;     // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack( pop )

inline void SetThreadName(std::thread& thread, const char* threadName) {
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = ::GetThreadId(reinterpret_cast<HANDLE>(thread.native_handle()));
    info.dwFlags = 0;

    __try {
        ::RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

namespace std {
    template <typename T>
    inline void hash_combine(std::size_t& seed, const T& v) {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    template<>
    struct hash<D3D12_SHADER_RESOURCE_VIEW_DESC> {
        std::size_t operator()(const D3D12_SHADER_RESOURCE_VIEW_DESC& srv_desc) const noexcept {
            std::size_t seed = 0;

            hash_combine(seed, srv_desc.Format);
            hash_combine(seed, srv_desc.ViewDimension);
            hash_combine(seed, srv_desc.Shader4ComponentMapping);

            switch (srv_desc.ViewDimension) {
                case D3D12_SRV_DIMENSION_BUFFER:
                    hash_combine(seed, srv_desc.Buffer.FirstElement);
                    hash_combine(seed, srv_desc.Buffer.NumElements);
                    hash_combine(seed, srv_desc.Buffer.StructureByteStride);
                    hash_combine(seed, srv_desc.Buffer.Flags);
                    break;
                case D3D12_SRV_DIMENSION_TEXTURE1D:
                    hash_combine(seed, srv_desc.Texture1D.MostDetailedMip);
                    hash_combine(seed, srv_desc.Texture1D.MipLevels);
                    hash_combine(seed, srv_desc.Texture1D.ResourceMinLODClamp);
                    break;
                case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
                    hash_combine(seed, srv_desc.Texture1DArray.MostDetailedMip);
                    hash_combine(seed, srv_desc.Texture1DArray.MipLevels);
                    hash_combine(seed, srv_desc.Texture1DArray.FirstArraySlice);
                    hash_combine(seed, srv_desc.Texture1DArray.ArraySize);
                    hash_combine(seed, srv_desc.Texture1DArray.ResourceMinLODClamp);
                    break;
                case D3D12_SRV_DIMENSION_TEXTURE2D:
                    hash_combine(seed, srv_desc.Texture2D.MostDetailedMip);
                    hash_combine(seed, srv_desc.Texture2D.MipLevels);
                    hash_combine(seed, srv_desc.Texture2D.PlaneSlice);
                    hash_combine(seed, srv_desc.Texture2D.ResourceMinLODClamp);
                    break;
                case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
                    hash_combine(seed, srv_desc.Texture2DArray.MostDetailedMip);
                    hash_combine(seed, srv_desc.Texture2DArray.MipLevels);
                    hash_combine(seed, srv_desc.Texture2DArray.FirstArraySlice);
                    hash_combine(seed, srv_desc.Texture2DArray.ArraySize);
                    hash_combine(seed, srv_desc.Texture2DArray.PlaneSlice);
                    hash_combine(seed, srv_desc.Texture2DArray.ResourceMinLODClamp);
                    break;
                case D3D12_SRV_DIMENSION_TEXTURE2DMS:
                    break;
                case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
                    hash_combine(seed, srv_desc.Texture2DMSArray.FirstArraySlice);
                    hash_combine(seed, srv_desc.Texture2DMSArray.ArraySize);
                    break;
                case D3D12_SRV_DIMENSION_TEXTURE3D:
                    hash_combine(seed, srv_desc.Texture3D.MostDetailedMip);
                    hash_combine(seed, srv_desc.Texture3D.MipLevels);
                    hash_combine(seed, srv_desc.Texture3D.ResourceMinLODClamp);
                    break;
                case D3D12_SRV_DIMENSION_TEXTURECUBE:
                    hash_combine(seed, srv_desc.TextureCube.MostDetailedMip);
                    hash_combine(seed, srv_desc.TextureCube.MipLevels);
                    hash_combine(seed, srv_desc.TextureCube.ResourceMinLODClamp);
                    break;
                case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
                    hash_combine(seed, srv_desc.TextureCubeArray.MostDetailedMip);
                    hash_combine(seed, srv_desc.TextureCubeArray.MipLevels);
                    hash_combine(seed, srv_desc.TextureCubeArray.First2DArrayFace);
                    hash_combine(seed, srv_desc.TextureCubeArray.NumCubes);
                    hash_combine(seed, srv_desc.TextureCubeArray.ResourceMinLODClamp);
                    break;
            }

            return seed;
        }
    };

    template<>
    struct hash<D3D12_UNORDERED_ACCESS_VIEW_DESC> {
        std::size_t operator()(const D3D12_UNORDERED_ACCESS_VIEW_DESC& uav_desc) const noexcept {
            std::size_t seed = 0;

            hash_combine(seed, uav_desc.Format);
            hash_combine(seed, uav_desc.ViewDimension);

            switch (uav_desc.ViewDimension) {
                case D3D12_UAV_DIMENSION_BUFFER:
                    hash_combine(seed, uav_desc.Buffer.FirstElement);
                    hash_combine(seed, uav_desc.Buffer.NumElements);
                    hash_combine(seed, uav_desc.Buffer.StructureByteStride);
                    hash_combine(seed, uav_desc.Buffer.CounterOffsetInBytes);
                    hash_combine(seed, uav_desc.Buffer.Flags);
                    break;
                case D3D12_UAV_DIMENSION_TEXTURE1D:
                    hash_combine(seed, uav_desc.Texture1D.MipSlice);
                    break;
                case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
                    hash_combine(seed, uav_desc.Texture1DArray.MipSlice);
                    hash_combine(seed, uav_desc.Texture1DArray.FirstArraySlice);
                    hash_combine(seed, uav_desc.Texture1DArray.ArraySize);
                    break;
                case D3D12_UAV_DIMENSION_TEXTURE2D:
                    hash_combine(seed, uav_desc.Texture2D.MipSlice);
                    hash_combine(seed, uav_desc.Texture2D.PlaneSlice);
                    break;
                case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
                    hash_combine(seed, uav_desc.Texture2DArray.MipSlice);
                    hash_combine(seed, uav_desc.Texture2DArray.FirstArraySlice);
                    hash_combine(seed, uav_desc.Texture2DArray.ArraySize);
                    hash_combine(seed, uav_desc.Texture2DArray.PlaneSlice);
                    break;
                case D3D12_UAV_DIMENSION_TEXTURE3D:
                    hash_combine(seed, uav_desc.Texture3D.MipSlice);
                    hash_combine(seed, uav_desc.Texture3D.FirstWSlice);
                    hash_combine(seed, uav_desc.Texture3D.WSize);
                    break;
            }

            return seed;
        }
    };
}

template<typename T>
constexpr const T& clamp(const T& val, const T& min, const T& max) {
    return val < min ? min : val > max ? max : val;
}

namespace Math {
    constexpr float PI = 3.1415926535897932384626433832795f;
    constexpr float _2PI = 2.0f * PI;
    
    constexpr float Degrees(const float radians) {
        return radians * (180.0f / PI);
    }

    constexpr float Radians(const float degrees) {
        return degrees * (PI / 180.0f);
    }

    template<typename T>
    inline T Deadzone(T val, T deadzone) {
        if (std::abs(val) < deadzone) {
            return T(0);
        }

        return val;
    }

    template<typename T, typename U>
    inline T NormalizeRange(U x, U min, U max) {
        return T(x - min) / T(max - min);
    }

    template<typename T, typename U>
    inline T ShiftBias(U x, U shift, U bias) {
        return T(x * bias) + T(shift);
    }

    template <typename T>
    inline T AlignUpWithMask(T value, size_t mask) {
        return (T)(((size_t)value + mask) & ~mask);
    }

    template <typename T>
    inline T AlignDownWithMask(T value, size_t mask) {
        return (T)((size_t)value & ~mask);
    }

    template <typename T>
    inline T AlignUp(T value, size_t alignment) {
        return AlignUpWithMask(value, alignment - 1);
    }

    template <typename T>
    inline T AlignDown(T value, size_t alignment) {
        return AlignDownWithMask(value, alignment - 1);
    }

    template <typename T>
    inline bool IsAligned(T value, size_t alignment) {
        return 0 == ((size_t)value & (alignment - 1));
    }

    template <typename T>
    inline T DivideByMultiple(T value, size_t alignment) {
        return (T)((value + alignment - 1) / alignment);
    }
    
    inline uint32_t NextHighestPow2(uint32_t v) {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v++;

        return v;
    }

    inline uint64_t NextHighestPow2(uint64_t v) {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v |= v >> 32;
        v++;

        return v;
    }
}

#define STR1(x) #x
#define STR(x) STR1(x)
#define WSTR1(x) L##x
#define WSTR(x) WSTR1(x)
#define NAME_D3D12_OBJECT(x) x->SetName( WSTR(__FILE__ "(" STR(__LINE__) "): " L#x) )