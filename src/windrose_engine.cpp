#include "windrose_engine.h"
#include "utils/pattern_finder.h"
#include "ban/ban_list.h"
#include "commands/commands.h"
#include <Psapi.h>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <Windows.h>

extern void LogMessage(const std::string& message);

#pragma comment(lib, "Psapi.lib")

std::string ReadServerInfo(const std::string& key) {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir(exePath);
    size_t lastSlash = exeDir.find_last_of("\\/");
    exeDir = exeDir.substr(0, lastSlash);
    
    std::string jsonPath = exeDir + "\\..\\..\\ServerDescription.json";
    
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        return "N/A";
    }
    
    std::string line;
    std::string searchKey = "\"" + key + "\":";
    while (std::getline(file, line)) {
        size_t pos = line.find(searchKey);
        if (pos != std::string::npos) {
            size_t valueStart = pos + searchKey.length();
            while (valueStart < line.length() && (line[valueStart] == ' ' || line[valueStart] == '\t')) {
                valueStart++;
            }
            
            if (line[valueStart] == '"') {
                size_t start = valueStart;
                size_t end = line.find("\"", start + 1);
                if (end != std::string::npos) {
                    return line.substr(start + 1, end - start - 1);
                }
            } else {
                size_t end = line.find_first_of(",\n\r", valueStart);
                if (end != std::string::npos) {
                    std::string value = line.substr(valueStart, end - valueStart);
                    while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
                        value.pop_back();
                    }
                    return value;
                }
            }
        }
    }
    return "N/A";
}

namespace UnrealEngine {
    StandaloneIntegration* g_Engine = nullptr;

    StandaloneIntegration::StandaloneIntegration() 
        : moduleBase(0), moduleSize(0), GObjectsPtr(0), GNamesPtr(0) {
    }

    StandaloneIntegration::~StandaloneIntegration() {
    }

    std::vector<uint8_t> StandaloneIntegration::FindPattern(const char* pattern, const char* mask) {
        std::vector<uint8_t> result;
        size_t patternLength = strlen(mask);
        
        for (size_t i = 0; i < moduleSize - patternLength; i++) {
            bool found = true;
            for (size_t j = 0; j < patternLength; j++) {
                if (mask[j] != '?' && pattern[j] != *reinterpret_cast<char*>(moduleBase + i + j)) {
                    found = false;
                    break;
                }
            }
            if (found) {
                for (size_t j = 0; j < 8; j++) {
                    result.push_back(*reinterpret_cast<uint8_t*>(moduleBase + i + j));
                }
                return result;
            }
        }
        return result;
    }

    uintptr_t StandaloneIntegration::FindGObjects() {
        return PatternScanner::ScanForGObjects(moduleBase, moduleSize);
    }

    uintptr_t StandaloneIntegration::FindGNames() {
        return 0;
    }

    bool StandaloneIntegration::Initialize() {
        HMODULE hModule = GetModuleHandleA(nullptr);
        if (!hModule) {
            LogMessage("Standalone: Failed to get module handle");
            return false;
        }

        MODULEINFO modInfo;
        if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO))) {
            LogMessage("Standalone: Failed to get module information");
            return false;
        }

        moduleBase = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
        moduleSize = modInfo.SizeOfImage;

        char hexBuffer[32];
        sprintf_s(hexBuffer, "0x%llX", moduleBase);
        LogMessage(std::string("Standalone: Module base: ") + hexBuffer);
        LogMessage("Standalone: Module size: " + std::to_string(moduleSize));
        
        GObjectsPtr = FindGObjects();
        
        if (GObjectsPtr) {
            sprintf_s(hexBuffer, "0x%llX", GObjectsPtr);
            LogMessage(std::string("GObjects found at: ") + hexBuffer);
        } else {
            LogMessage("WARNING: GObjects not found - player commands will not work");
        }
        
        LogMessage("Standalone: Integration initialized");
        return true;
    }

    std::vector<PlayerInfo> StandaloneIntegration::GetAllPlayers() {
        std::vector<PlayerInfo> players;
        
        // Get UWorld from GWorld offset
        // Reference: SDK/Basic.hpp - Offsets::GWorld = 0x0F62F460
        uintptr_t gWorldAddr = moduleBase + 0x0F62F460;
        UObject** gWorldPtr = (UObject**)gWorldAddr;
        
        if (!gWorldPtr || IsBadReadPtr(gWorldPtr, 8) || !*gWorldPtr) {
            LogMessage("GWorld not available");
            return players;
        }
        
        UObject* world = *gWorldPtr;
        
        // UWorld->GameState is at offset 0x01B0 (for dedicated servers)
        // Reference: SDK/Engine_classes.hpp - UWorld::GameState
        UObject** gameStatePtr = (UObject**)((uintptr_t)world + 0x01B0);
        if (!gameStatePtr || IsBadReadPtr(gameStatePtr, 8) || !*gameStatePtr) {
            LogMessage("GameState not available");
            return players;
        }
        
        UObject* gameState = *gameStatePtr;
        
        // GameState->PlayerArray is a TArray at offset 0x02C0
        // Reference: SDK/Engine_classes.hpp - AGameStateBase::PlayerArray
        struct TArray {
            void** Data;
            int32_t Count;
            int32_t Max;
        };
        
        TArray* playerArray = (TArray*)((uintptr_t)gameState + 0x02C0);
        if (!playerArray || IsBadReadPtr(playerArray, sizeof(TArray))) {
            LogMessage("PlayerArray not available");
            return players;
        }
        
        
        // Iterate through PlayerArray
        for (int i = 0; i < playerArray->Count; i++) {
            UObject** playerStatePtr = (UObject**)playerArray->Data;
            if (!playerStatePtr || IsBadReadPtr(playerStatePtr, 8 * (i + 1))) continue;
            
            UObject* playerState = playerStatePtr[i];
            if (!playerState || IsBadReadPtr(playerState, 0x400)) continue;
            
            // Find PlayerController for this PlayerState
            // Simple approach: Get from PlayerState->PawnPrivate->Controller
            UObject* playerController = nullptr;
            
            // PlayerState->PawnPrivate is at offset 0x0320
            // Reference: SDK/Engine_classes.hpp - APlayerState::PawnPrivate
            UObject** pawnPtr = (UObject**)((uintptr_t)playerState + 0x0320);
            if (pawnPtr && !IsBadReadPtr(pawnPtr, 8) && *pawnPtr) {
                UObject* pawn = *pawnPtr;
                
                // APawn->Controller is at offset 0x02D8
                // Reference: SDK/Engine_classes.hpp - APawn::Controller at 0x02D8
                UObject** controllerPtr = (UObject**)((uintptr_t)pawn + 0x02D8);
                if (controllerPtr && !IsBadReadPtr(controllerPtr, 8) && *controllerPtr) {
                    playerController = *controllerPtr;
                }
            }
            
            if (!playerController) {
            }
            
            PlayerInfo info = {};
            info.playerStatePtr = (uintptr_t)playerState;
            info.playerControllerPtr = (uintptr_t)playerController;

            // Read pawn location via RootComponent
            // AActor::RootComponent at 0x01B8, USceneComponent::RelativeLocation at 0x0140 (FVector double[3])
            if (pawnPtr && !IsBadReadPtr(pawnPtr, 8) && *pawnPtr) {
                UObject* pawn = *pawnPtr;
                info.pawnPtr = (uintptr_t)pawn;
                UObject** rootCompPtr = (UObject**)((uintptr_t)pawn + 0x01B8);
                if (!IsBadReadPtr(rootCompPtr, 8) && *rootCompPtr) {
                    uintptr_t rootComp = (uintptr_t)*rootCompPtr;
                    double* loc = (double*)(rootComp + 0x0140);
                    double* rot = (double*)(rootComp + 0x0158);
                    if (!IsBadReadPtr(loc, sizeof(double) * 3) && !IsBadReadPtr(rot, sizeof(double) * 3)) {
                        info.x = loc[0]; info.y = loc[1]; info.z = loc[2];
                        info.pitch = rot[0]; info.yaw = rot[1]; info.roll = rot[2];
                        info.hasLocation = true;
                    }
                }
            }
            
            // PlayerNamePrivate offset in APlayerState
            // Reference: SDK/Engine_classes.hpp - APlayerState::PlayerNamePrivate at 0x0340
            FString* namePtr = (FString*)((uintptr_t)playerState + 0x0340);
            if (namePtr && !IsBadReadPtr(namePtr, sizeof(FString)) && namePtr->Length > 0) {
                info.playerName = namePtr->ToString();
            }
            
            // AccountId offset in AR5DataKeeper_PlayerState
            // Reference: SDK/R5DataKeepers_classes.hpp - AR5DataKeeper_PlayerState::AccountData at 0x0378
            // Reference: SDK/R5DataKeepers_structs.hpp - FR5DataKeeper_AccountData::AccountId at +0x0010
            // Total offset: 0x0378 + 0x0010 = 0x0388
            FString* idPtr = (FString*)((uintptr_t)playerState + 0x0388);
            if (idPtr && !IsBadReadPtr(idPtr, sizeof(FString)) && idPtr->Length > 0) {
                std::wstring idW = idPtr->ToString();
                info.accountId = std::string(idW.begin(), idW.end());
            }
            
            players.push_back(info);
        }
        
        return players;
    }

    // Pattern scan for function in .text section
    void* FindFunctionByPattern(const char* pattern, const char* mask, size_t patternSize) {
        uintptr_t moduleBase = (uintptr_t)GetModuleHandleA(NULL);
        PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)moduleBase;
        PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(moduleBase + dosHeader->e_lfanew);
        
        PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(ntHeaders);
        for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, section++) {
            if (strcmp((char*)section->Name, ".text") == 0) {
                uintptr_t start = moduleBase + section->VirtualAddress;
                size_t size = section->Misc.VirtualSize;
                
                for (size_t j = 0; j < size - patternSize; j++) {
                    bool found = true;
                    for (size_t k = 0; k < patternSize; k++) {
                        if (mask[k] == 'x' && ((unsigned char*)start)[j + k] != (unsigned char)pattern[k]) {
                            found = false;
                            break;
                        }
                    }
                    if (found) {
                        char msg[256];
                        sprintf_s(msg, "Pattern found at offset: 0x%llX", (start + j) - moduleBase);
                        LogMessage(msg);
                        return (void*)(start + j);
                    }
                }
            }
        }
        return nullptr;
    }

    bool StandaloneIntegration::KickPlayer(const std::string& accountId) {
        LogMessage("Standalone: Kick request for " + accountId);
        
        auto players = GetAllPlayers();
        
        for (const auto& player : players) {
            if (player.accountId.find(accountId) != std::string::npos) {
                std::string name(player.playerName.begin(), player.playerName.end());
                LogMessage("Attempting to kick: " + name);
                
                UObject* playerController = (UObject*)player.playerControllerPtr;
                if (!playerController) {
                    LogMessage("PlayerController is null");
                    return false;
                }
                
                UObject* pcClass = (UObject*)playerController->ClassPrivate;
                
                struct FString {
                    wchar_t* Data;
                    int32_t Count;
                    int32_t Max;
                };
                
                // Reference: SDK/Basic.hpp - Offsets::AppendString = 0x014551E0
                typedef void(*AppendStringFn)(void*, FString*);
                AppendStringFn AppendString = (AppendStringFn)(moduleBase + 0x014551E0);
                
                UObject* clientTravelFunc = nullptr;
                
                for (UObject* clss = pcClass; clss && !IsBadReadPtr(clss, 0x100); ) {
                    // Reference: SDK/CoreUObject_classes.hpp - UStruct::Children = 0x0048
                    UObject** childrenPtr = (UObject**)((uintptr_t)clss + 0x0048);
                    if (childrenPtr && !IsBadReadPtr(childrenPtr, 8)) {
                        for (UObject* field = *childrenPtr; field && !IsBadReadPtr(field, 0x50); ) {
                            wchar_t nameBuf[256] = {0};
                            FString nameStr = {nameBuf, 0, 256};
                            // Reference: SDK/CoreUObject_classes.hpp - UField::NamePrivate = 0x0018
                            void* fnamePtr = (void*)((uintptr_t)field + 0x0018);
                            
                            try {
                                AppendString(fnamePtr, &nameStr);
                                if (wcscmp(nameBuf, L"ClientTravelInternal") == 0) {
                                    clientTravelFunc = field;
                                    LogMessage("Found ClientTravelInternal");
                                    break;
                                }
                            } catch (...) {}
                            
                            // Reference: SDK/CoreUObject_classes.hpp - UField::Next = 0x0028
                            UObject** nextPtr = (UObject**)((uintptr_t)field + 0x0028);
                            if (!nextPtr || IsBadReadPtr(nextPtr, 8)) break;
                            field = *nextPtr;
                        }
                    }
                    if (clientTravelFunc) break;
                    
                    // Reference: SDK/CoreUObject_classes.hpp - UStruct::SuperStruct = 0x0040
                    UObject** superPtr = (UObject**)((uintptr_t)clss + 0x0040);
                    if (!superPtr || IsBadReadPtr(superPtr, 8)) break;
                    clss = *superPtr;
                }
                
                if (!clientTravelFunc) {
                    LogMessage("ClientTravelInternal not found");
                    return false;
                }
                
                // Reference: SDK/Basic.hpp - Offsets::ProcessEvent = 0x01692A00
                typedef void(*ProcessEventFn)(UObject*, UObject*, void*);
                ProcessEventFn ProcessEvent = (ProcessEventFn)(moduleBase + 0x01692A00);
                
                struct {
                    FString URL;
                    uint8_t TravelType;
                    bool bSeamless;
                    uint8_t Pad_12[2];
                    uint32_t MapPackageGuid[4];
                    uint8_t Pad_24[4];
                } params = {};
                
                // "Void" now corrupts the player's data...
                // Using "/Game/Maps/Lobby/R5ClientLobby.R5ClientLobby_C" to send the player to the lobby
                static wchar_t lobbyUrl[] = L"/Game/Maps/Lobby/R5ClientLobby.R5ClientLobby_C";
                params.URL.Data = lobbyUrl;
                params.URL.Count = (int32_t)(sizeof(lobbyUrl) / sizeof(wchar_t));
                params.URL.Max = params.URL.Count;
                params.TravelType = 0;
                params.bSeamless = false;
                
                LogMessage("Calling ClientTravelInternal");
                ProcessEvent(playerController, clientTravelFunc, &params);
                
                LogMessage("Player kicked via ClientTravelInternal");
                return true;
            }
        }
        
        LogMessage("Player not found: " + accountId);
        return false;
    }

    std::string StandaloneIntegration::ExecuteCommand(const std::string& command) {
        std::string cmd = command;
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
        
        if (cmd == "help") {
            return Commands::ExecuteHelp();
        }
        else if (cmd == "info") {
            return Commands::ExecuteInfo();
        }
        else if (cmd == "showplayers") {
            return Commands::ExecuteShowPlayers();
        }
        else if (cmd == "kick" || cmd.substr(0, 5) == "kick ") {
            std::string args = cmd.size() > 5 ? command.substr(5) : "";
            return Commands::ExecuteKick(args);
        }
        else if (cmd == "ban" || cmd.substr(0, 4) == "ban ") {
            std::string args = cmd.size() > 4 ? command.substr(4) : "";
            return Commands::ExecuteBan(args);
        }
        else if (cmd == "unban" || cmd.substr(0, 6) == "unban ") {
            std::string args = cmd.size() > 6 ? command.substr(6) : "";
            return Commands::ExecuteUnban(args);
        }
        else if (cmd == "getpos" || cmd.substr(0, 7) == "getpos ") {
            std::string args = cmd.size() > 7 ? command.substr(7) : "";
            return Commands::ExecuteGetPos(args);
        }
        else if (cmd == "banlist") {
            return Commands::ExecuteBanlist();
        }
        else if (cmd == "shutdown" || cmd.substr(0, 9) == "shutdown ") {
            std::string args = cmd.size() > 9 ? command.substr(9) : "";
            return Commands::Shutdown(args);
        }
        else if (cmd == "uptime") {
            return Commands::ExecuteUptime();
        }
        else if (cmd == "playerinfo" || cmd.substr(0, 11) == "playerinfo ") {
            std::string args = cmd.size() > 11 ? command.substr(11) : "";
            return Commands::ExecutePlayerInfo(args);
        }
        else {
            std::stringstream response;
            response << "Unknown command: " << command << "\n";
            response << "Type 'help' for available commands\n";
            return response.str();
        }
    }
}

std::string ExecuteRCONCommand(const std::string& command) {
    LogMessage("ExecuteRCONCommand called with: " + command);
    
    if (!UnrealEngine::g_Engine) {
        LogMessage("ERROR: g_Engine is nullptr!");
        return "ERROR: Engine pointer is null. Command: " + command;
    }
    
    if (!UnrealEngine::g_Engine->IsInitialized()) {
        LogMessage("ERROR: Engine not initialized!");
        return "ERROR: Engine not initialized. Command: " + command;
    }
    
    LogMessage("Executing command via engine...");
    return UnrealEngine::g_Engine->ExecuteCommand(command);
}
