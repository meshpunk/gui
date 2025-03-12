# MeshPunk - LVGL with Lua for T-Deck

This project demonstrates using [LuaVGL](https://github.com/XuNeo/luavgl) on the LilyGo T-Deck device. LuaVGL is a Lua binding for LVGL that allows you to create GUIs with Lua scripts.

## Features

- Combines the power of LVGL with the simplicity of Lua scripting
- Runs on the LilyGo T-Deck hardware
- Demonstrates touch and display capabilities
- Uses PlatformIO for easy building and deployment

## Requirements

- PlatformIO
- T-Deck device
- Git (for submodules)

## Building

1. Clone this repository
2. Initialize the submodules:
   ```
   git submodule update --init --recursive
   ```
3. Open in PlatformIO
4. Build and upload to your T-Deck device

## Submodules

This project uses the following Git submodules:
- [LuaVGL](https://github.com/XuNeo/luavgl) - Located in `lib/luavgl`

## Usage

The example displays a "Hello MeshPunk World!" message and a button that, when clicked, changes the message text.

## License

MIT

## Credits

- LuaVGL by XuNeo: https://github.com/XuNeo/luavgl
- LVGL: https://lvgl.io/
- LilyGo for the T-Deck hardware