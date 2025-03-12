# MeshPunk - LVGL with Lua for T-Deck

This project demonstrates using [LuaVGL](https://github.com/XuNeo/luavgl) on the LilyGo T-Deck device. LuaVGL is a Lua binding for LVGL that allows you to create GUIs with Lua scripts.

## Features

- Combines the power of LVGL with the simplicity of Lua scripting
- Runs on the LilyGo T-Deck hardware
- Demonstrates touch and display capabilities
- Uses PlatformIO for easy building and deployment
- Loads Lua scripts from the filesystem for easy development

## Project Structure

- `/src` - Main C++ code
  - `main.cpp` - Main application code
  - `utilities.h` - T-Deck pin definitions and utilities
- `/data` - Data files that get uploaded to the device filesystem
  - `/lua` - Lua scripts
    - `messenger.lua` - Main messenger application
    - `utils.lua` - Utility functions
  - `/sounds` - Sound files
  - `/images` - Image files

## Requirements

- PlatformIO
- T-Deck device
- Git (for submodules)

## Building and Development

1. Clone this repository
2. Initialize the submodules:
   ```
   git submodule update --init --recursive
   ```
3. Open in PlatformIO
4. Edit Lua scripts in the `/data/lua` directory
5. Build and upload to your T-Deck device:
   ```
   pio run --target upload
   ```
   This will automatically upload both the firmware and the filesystem data.
6. To upload only the filesystem data (after changing Lua scripts):
   ```
   pio run --target uploadfs
   ```

## Submodules

This project uses the following Git submodules:
- [LuaVGL](https://github.com/XuNeo/luavgl) - Located in `lib/luavgl`

## Usage

The example loads the `messenger.lua` script from the filesystem and displays a simple messenger UI. You can edit the Lua scripts in your IDE with proper syntax highlighting and then upload just the filesystem to quickly iterate on your UI design.

### Developing Lua Scripts

1. Edit the Lua scripts in `/data/lua`
2. Upload the filesystem with `pio run --target uploadfs`
3. The device will automatically load the updated scripts

### Adding Additional Scripts

You can create additional Lua scripts in the `/data/lua` directory. Scripts can be loaded from other scripts using `require`:

```lua
local utils = require('utils')
```

## License

MIT

## Credits

- LuaVGL by XuNeo: https://github.com/XuNeo/luavgl
- LVGL: https://lvgl.io/
- LilyGo for the T-Deck hardware