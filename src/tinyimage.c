#include <lua.h>
#include <lauxlib.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    uint8_t width;
    uint8_t height;
    uint8_t palette[4][3];  // 4 colors, each R,G,B
    uint8_t pixels[];       // flexible array for pixel data
} TinyImage;

// Create a new TinyImage
static int tinyimage_new(lua_State *L) {
    int width = luaL_checkinteger(L, 1);
    int height = luaL_checkinteger(L, 2);
    
    if (width <= 0 || width > 255 || height <= 0 || height > 255) {
        return luaL_error(L, "width and height must be between 1 and 255");
    }
    
    // Allocate memory for the image
    size_t size = sizeof(TinyImage) + (width * height + 1) / 2;  // +1/2 for rounding up
    TinyImage *img = (TinyImage *)lua_newuserdata(L, size);
    
    // Set the metatable
    luaL_getmetatable(L, "TinyImage");
    lua_setmetatable(L, -2);
    
    // Initialize the image
    img->width = width;
    img->height = height;
    memset(img->pixels, 0, (width * height + 1) / 2);  // Initialize all pixels to 0
    
    // Set default palette (black, white, red, blue)
    img->palette[0][0] = 0;   img->palette[0][1] = 0;   img->palette[0][2] = 0;    // Black
    img->palette[1][0] = 255; img->palette[1][1] = 255; img->palette[1][2] = 255;  // White
    img->palette[2][0] = 255; img->palette[2][1] = 0;   img->palette[2][2] = 0;    // Red
    img->palette[3][0] = 0;   img->palette[3][1] = 0;   img->palette[3][2] = 255;  // Blue
    
    return 1;
}

// Set a pixel's color
static int tinyimage_set_pixel(lua_State *L) {
    TinyImage *img = (TinyImage *)luaL_checkudata(L, 1, "TinyImage");
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int color = luaL_checkinteger(L, 4);
    
    if (x < 0 || x >= img->width || y < 0 || y >= img->height) {
        return luaL_error(L, "coordinates out of bounds");
    }
    if (color < 0 || color > 3) {
        return luaL_error(L, "color index must be between 0 and 3");
    }
    
    int index = y * img->width + x;
    int byte_index = index / 2;
    int bit_offset = (index % 2) * 2;
    
    // Clear the 2 bits for this pixel
    img->pixels[byte_index] &= ~(3 << bit_offset);
    // Set the new color
    img->pixels[byte_index] |= (color << bit_offset);
    
    return 0;
}

// Get a pixel's color
static int tinyimage_get_pixel(lua_State *L) {
    TinyImage *img = (TinyImage *)luaL_checkudata(L, 1, "TinyImage");
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    
    if (x < 0 || x >= img->width || y < 0 || y >= img->height) {
        return luaL_error(L, "coordinates out of bounds");
    }
    
    int index = y * img->width + x;
    int byte_index = index / 2;
    int bit_offset = (index % 2) * 2;
    
    int color = (img->pixels[byte_index] >> bit_offset) & 3;
    lua_pushinteger(L, color);
    
    return 1;
}

// Set a palette color
static int tinyimage_set_palette(lua_State *L) {
    TinyImage *img = (TinyImage *)luaL_checkudata(L, 1, "TinyImage");
    int index = luaL_checkinteger(L, 2);
    int r = luaL_checkinteger(L, 3);
    int g = luaL_checkinteger(L, 4);
    int b = luaL_checkinteger(L, 5);
    
    if (index < 0 || index > 3) {
        return luaL_error(L, "palette index must be between 0 and 3");
    }
    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
        return luaL_error(L, "RGB values must be between 0 and 255");
    }
    
    img->palette[index][0] = r;
    img->palette[index][1] = g;
    img->palette[index][2] = b;
    
    return 0;
}

// Get a palette color
static int tinyimage_get_palette(lua_State *L) {
    TinyImage *img = (TinyImage *)luaL_checkudata(L, 1, "TinyImage");
    int index = luaL_checkinteger(L, 2);
    
    if (index < 0 || index > 3) {
        return luaL_error(L, "palette index must be between 0 and 3");
    }
    
    lua_pushinteger(L, img->palette[index][0]);
    lua_pushinteger(L, img->palette[index][1]);
    lua_pushinteger(L, img->palette[index][2]);
    
    return 3;
}

// Get image dimensions
static int tinyimage_get_size(lua_State *L) {
    TinyImage *img = (TinyImage *)luaL_checkudata(L, 1, "TinyImage");
    lua_pushinteger(L, img->width);
    lua_pushinteger(L, img->height);
    return 2;
}

static const luaL_Reg tinyimage_methods[] = {
    {"new", tinyimage_new},
    {"set_pixel", tinyimage_set_pixel},
    {"get_pixel", tinyimage_get_pixel},
    {"set_palette", tinyimage_set_palette},
    {"get_palette", tinyimage_get_palette},
    {"get_size", tinyimage_get_size},
    {NULL, NULL}
};

int luaopen_tinyimage(lua_State *L) {
    // Create metatable
    luaL_newmetatable(L, "TinyImage");
    
    // Set metatable methods
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    
    // Register methods
    luaL_setfuncs(L, tinyimage_methods, 0);
    
    // Create the module table
    lua_newtable(L);
    luaL_setfuncs(L, tinyimage_methods, 0);
    
    return 1;
} 