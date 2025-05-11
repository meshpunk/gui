#include <lua.h>
#include <lauxlib.h>
#include <stdint.h>
#include <string.h>
#include <lvgl.h>
#include "luavgl.h"

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

    // Check if palette table was provided
    if (lua_istable(L, 3)) {
        lua_len(L, 3);
        int palette_size = lua_tointeger(L, -1);
        lua_pop(L, 1);
        
        if (palette_size != 4) {
            return luaL_error(L, "Palette must be a table with 4 colors");
        }
        
        for (int i = 0; i < 4; i++) {
            lua_pushinteger(L, i + 1);
            lua_gettable(L, 3);
            
            if (!lua_isstring(L, -1)) {
                return luaL_error(L, "Color must be a hex string");
            }
            
            const char* hex = lua_tostring(L, -1);
            if (strlen(hex) != 7 || hex[0] != '#') {
                return luaL_error(L, "Color must be a hex string in format #RRGGBB");
            }
            
            // Convert hex to RGB
            char hex_r[3] = {hex[1], hex[2], 0};
            char hex_g[3] = {hex[3], hex[4], 0};
            char hex_b[3] = {hex[5], hex[6], 0};
            
            img->palette[i][0] = strtol(hex_r, NULL, 16);
            img->palette[i][1] = strtol(hex_g, NULL, 16);
            img->palette[i][2] = strtol(hex_b, NULL, 16);
            
            lua_pop(L, 1);
        }
    }
    
    return 1;
}

// Set a pixel's color
static int tinyimage_set_pixel(lua_State *L) {
    TinyImage *img = (TinyImage *)luaL_checkudata(L, 1, "TinyImage");
    int x = luaL_checkinteger(L, 2) - 1;
    int y = luaL_checkinteger(L, 3) - 1;
    int color = luaL_checkinteger(L, 4) - 1;
    
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
    int x = luaL_checkinteger(L, 2) - 1;
    int y = luaL_checkinteger(L, 3) - 1;
    
    if (x < 0 || x >= img->width || y < 0 || y >= img->height) {
        return luaL_error(L, "coordinates out of bounds");
    }
    
    int index = y * img->width + x;
    int byte_index = index / 2;
    int bit_offset = (index % 2) * 2;
    
    int color = ((img->pixels[byte_index] >> bit_offset) & 3) + 1;
    lua_pushinteger(L, color);
    
    return 1;
}

// Set a palette color
static int tinyimage_set_palette(lua_State *L) {
    TinyImage *img = (TinyImage *)luaL_checkudata(L, 1, "TinyImage");
    int index = luaL_checkinteger(L, 2) - 1;
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
    int index = luaL_checkinteger(L, 2) - 1;
    
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

// Draw the image to an LVGL object
static int tinyimage_draw(lua_State *L) {
    TinyImage *img = (TinyImage *)luaL_checkudata(L, 1, "TinyImage");
    lv_obj_t* obj = luavgl_to_obj(L, 2);
    
    if (!obj) {
        return luaL_error(L, "Invalid LVGL object");
    }
    
    int width = img->width;
    int height = img->height;
    
    // Get object dimensions from style if not laid out yet
    int obj_width = lv_obj_get_style_width(obj, LV_PART_MAIN);
    int obj_height = lv_obj_get_style_height(obj, LV_PART_MAIN);

    int scale = obj_width / width;
    
    if (scale * width != obj_width || scale * height != obj_height) {
        return luaL_error(L, "Object scale must be an integer and consistent in x and y.\nGot object width=%d, height=%d, while the image is width=%d, height=%d", 
            obj_width, obj_height, width, height);
    }
    printf("Drawing image as %d x %d\n", width * scale, height * scale);
    
    // Clear any existing children
    lv_obj_clean(obj);
    
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            int color_idx = ((img->pixels[(j * width + i) / 2] >> ((j * width + i) % 2 * 2)) & 3);
            uint8_t r = img->palette[color_idx][0];
            uint8_t g = img->palette[color_idx][1];
            uint8_t b = img->palette[color_idx][2];
            
            lv_obj_t* pixel = lv_obj_create(obj);
            lv_obj_set_pos(pixel, i * scale, j * scale);
            lv_obj_set_size(pixel, scale, scale);
            lv_obj_set_style_bg_color(pixel, lv_color_make(r, g, b), LV_STATE_DEFAULT);
            lv_obj_set_style_radius(pixel, 0, LV_PART_MAIN);
            lv_obj_set_style_border_width(pixel, 0, LV_PART_MAIN);
            lv_obj_clear_flag(pixel, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_clear_flag(pixel, LV_OBJ_FLAG_CLICKABLE);
            
            // Force immediate redraw
            lv_obj_invalidate(pixel);
        }
    }
    
    // Force parent to redraw
    lv_obj_invalidate(obj);
    
    return 0;
}

static const luaL_Reg tinyimage_methods[] = {
    {"new", tinyimage_new},
    {"set_pixel", tinyimage_set_pixel},
    {"get_pixel", tinyimage_get_pixel},
    {"set_palette", tinyimage_set_palette},
    {"get_palette", tinyimage_get_palette},
    {"get_size", tinyimage_get_size},
    {"draw", tinyimage_draw},
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