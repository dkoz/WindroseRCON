#pragma once
#include <cstdint>

// All game offsets used by the RCON mod, taken from the Dumper-7 SDK.
// SDK version: 5.6.1-0+UE5-R5
//
// Update these after every Windrose patch, then rebuild. Nothing else in the
// mod hardcodes an offset.

namespace Offsets {

    // Module-base relative globals
    // Reference: SDK/Basic.hpp - namespace Offsets
    namespace Global {
        constexpr uintptr_t GObjects     = 0x0FB25050;
        constexpr uintptr_t AppendString = 0x01455320;
        constexpr uintptr_t GNames       = 0x0FA40DC0;
        constexpr uintptr_t GWorld       = 0x0F635460;
        constexpr uintptr_t ProcessEvent = 0x01692C30;
    }

    // Reference: SDK/CoreUObject_classes.hpp - Class CoreUObject.Object
    namespace UObject {
        constexpr uintptr_t Flags = 0x0008;
        constexpr uintptr_t Index = 0x000C;
        constexpr uintptr_t Class = 0x0010;
        constexpr uintptr_t Name  = 0x0018;
        constexpr uintptr_t Outer = 0x0020;
    }

    // Reference: SDK/CoreUObject_classes.hpp - Class CoreUObject.Field
    namespace UField {
        constexpr uintptr_t Next = 0x0028;
    }

    // Reference: SDK/CoreUObject_classes.hpp - Class CoreUObject.Struct
    namespace UStruct {
        constexpr uintptr_t SuperStruct = 0x0040;
        constexpr uintptr_t Children    = 0x0048;
    }

    // Reference: SDK/Engine_classes.hpp - Class Engine.World
    namespace UWorld {
        constexpr uintptr_t GameState = 0x01B0;
    }

    // Reference: SDK/Engine_classes.hpp - Class Engine.GameStateBase
    namespace AGameStateBase {
        constexpr uintptr_t PlayerArray = 0x02C0;
    }

    // Reference: SDK/Engine_classes.hpp - Class Engine.Actor
    namespace AActor {
        constexpr uintptr_t RootComponent = 0x01B8;
    }

    // Reference: SDK/Engine_classes.hpp - Class Engine.SceneComponent
    namespace USceneComponent {
        constexpr uintptr_t RelativeLocation = 0x0140;
        constexpr uintptr_t RelativeRotation = 0x0158;
    }

    // Reference: SDK/Engine_classes.hpp - Class Engine.Pawn
    namespace APawn {
        constexpr uintptr_t Controller = 0x02D8;
    }

    // Reference: SDK/Engine_classes.hpp - Class Engine.PlayerState
    namespace APlayerState {
        constexpr uintptr_t PawnPrivate       = 0x0320;
        constexpr uintptr_t PlayerNamePrivate = 0x0340;

        // Bitfield byte holding bIsSpectator / bOnlySpectator / bIsInactive
        constexpr uintptr_t Flags             = 0x02B2;
        constexpr uint8_t   Bit_bOnlySpectator = 1 << 2;
        constexpr uint8_t   Bit_bIsInactive    = 1 << 5;
    }

    // Reference: SDK/R5DataKeepers_classes.hpp - Class R5DataKeepers.R5DataKeeper_PlayerState
    // Reference: SDK/R5DataKeepers_structs.hpp - FR5DataKeeper_AccountData
    namespace AR5DataKeeper_PlayerState {
        constexpr uintptr_t AccountData = 0x0378;

        namespace AccountData_ {
            constexpr uintptr_t RegionId  = 0x0000;
            constexpr uintptr_t AccountId = 0x0010;
        }

        constexpr uintptr_t AccountId = AccountData + AccountData_::AccountId;
    }
}
