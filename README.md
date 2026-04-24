# Windrose RCON

Standalone RCON server for Windrose game servers using source rcon protocol. Injects via `version.dll` proxy and provides server information, query and player management.

## Features

- **Server Information** - View server details, player count, invite code
- **Player List** - See all connected players with their account IDs
- **Configuration** - Settings stored in `windrosercon/settings.ini`
- **Logging** - All activity logged to `windrosercon/rcon.log`
- **Player Management** - Manage players by their account IDs.

## Installation

1. Extract the `version.dll` to your server's Win64 folder:
   ```
   YourServer/R5/Binaries/Win64/version.dll
   ```

2. Start your server, RCON will initialize automatically.

3. Configuration file will be created at:
   ```
   YourServer/R5/Binaries/Win64/windrosercon/settings.ini
   ```

## Configuration

Edit `windrosercon/settings.ini`:

```ini
[RCON]
Port=27065
Password=windrose_admin
EnableLogging=true
LogFile=windrosercon\rcon.log
```

## Usage

Connect with any Source RCON client (e.g., `test_rcon.py`):

```bash
python test_rcon.py
```

### Available Commands

- `info` - Display server information
- `showplayers` - List all online players
- `help` - Show available commands
- `kick <AccountId>` - Kicks player by their account ID
- `ban <AccountId>` - Bans player by their account ID
- `unban <AccountId>` - Unbans player by their account ID
- `banlist` - Shows all banned players
- `getpos <AccountId>` - Gets player world position by their account ID

## Building

### Requirements
- xmake v3.0.2+
- Visual Studio 2022
- Windows SDK

## Build Project
1. Build the project with xmake:
   ```bash
   xmake
   ```

## Technical Details

### How It Works

1. **DLL Proxy Injection**: `version.dll` acts as a proxy for the system `version.dll`, forwarding all API calls while injecting our code
2. **GObjects Scanning**: Scans Unreal Engine's global object array to find player states
3. **Player Validation**: Validates players by checking offsets:
   - `0x0340` - PlayerName (FString)
   - `0x0388` - AccountId (FString)  
   - `0x02B2` - PlayerFlags (filters inactives)
4. **RCON Protocol**: Implements Source RCON protocol for authentication and commands.

### Credits
- [Dumper7](https://github.com/Encryqed/Dumper-7) - For SDK generation
- [GameHostBros](https://www.gamehostbros.com/) - Providing server for testing on Wine.