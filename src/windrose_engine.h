#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <map>

// FString with Small String Optimization (SSO)
// Reference: SDK/UnrealContainers.hpp
struct FString {
    union {
        wchar_t* Data;
        wchar_t InlineData[12];
    };
    int32_t Length;
    int32_t Max;
    
    bool IsInline() const { return Max == 0; }
    
    const wchar_t* GetData() const {
        return IsInline() ? InlineData : Data;
    }
    
    std::wstring ToString() const {
        if (Length <= 0) return L"";
        return std::wstring(GetData(), Length);
    }
    
    FString(const wchar_t* str) {
        Length = wcslen(str);
        if (Length < 12) {
            Max = 0;
            wcscpy_s(InlineData, 12, str);
        } else {
            Max = Length + 1;
            Data = new wchar_t[Max];
            wcscpy_s(Data, Max, str);
        }
    }
    
    FString() : Data(nullptr), Length(0), Max(0) {}
};

// UObject structure
// Reference: SDK/CoreUObject_classes.hpp - Class CoreUObject.Object (0x0028 size)
struct UObject {
    void** VTable;                  // 0x0000
    int32_t ObjectFlags;            // 0x0008 (EObjectFlags)
    int32_t InternalIndex;          // 0x000C (Index)
    void* ClassPrivate;             // 0x0010 (UClass*)
    void* NamePrivate;              // 0x0018 (FName)
    void* OuterPrivate;             // 0x0020 (UObject*)
    
    typedef void(__fastcall* ProcessEventFn)(UObject*, void*, void*);
    
    // ProcessEvent function
    // Reference: SDK/Basic.hpp - Offsets::ProcessEvent = 0x01692A00
    void ProcessEvent(void* function, void* params) {
        static uintptr_t moduleBase = 0;
        if (!moduleBase) {
            moduleBase = (uintptr_t)GetModuleHandleA(NULL);
        }
        ProcessEventFn fn = (ProcessEventFn)(moduleBase + 0x01692A00);
        fn(this, function, params);
    }
};

struct UFunction {
    void* VTable;
};

struct PlayerInfo {
    std::wstring playerName;
    std::string accountId;
    uintptr_t playerStatePtr;
    uintptr_t playerControllerPtr;
    uintptr_t pawnPtr;
    bool hasLocation;
    double x, y, z;
    double pitch, yaw, roll;
};

namespace UnrealEngine {
    class StandaloneIntegration {
    private:
        uintptr_t moduleBase;
        size_t moduleSize;
        
        uintptr_t GObjectsPtr;
        uintptr_t GNamesPtr;
        
        std::vector<uint8_t> FindPattern(const char* pattern, const char* mask);
        uintptr_t FindGObjects();
        uintptr_t FindGNames();
        
    public:
        StandaloneIntegration();
        ~StandaloneIntegration();
        
        bool Initialize();
        std::string ExecuteCommand(const std::string& command);
        std::vector<PlayerInfo> GetAllPlayers();
        bool KickPlayer(const std::string& accountId);
        bool IsInitialized() const { return moduleBase != 0; }
    };

    extern StandaloneIntegration* g_Engine;
}
