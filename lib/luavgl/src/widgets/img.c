#include "luavgl.h"
#include "private.h"
#include "rotable.h"

static int luavgl_img_create(lua_State *L)
{
  return luavgl_obj_create_helper(L, lv_image_create);
}

static int image_set_pivot(lua_State *L, lv_obj_t *obj, bool set)
{
  if (!set) {
    /* Read pivot */
    lv_point_t p;
    lv_image_get_pivot(obj, &p);
    lua_createtable(L, 0, 2);
    lua_pushinteger(L, p.x);
    lua_seti(L, -2, p.x);
    lua_pushinteger(L, p.y);
    lua_seti(L, -2, p.y);
    return 1;
  }

  lv_point_t point = luavgl_topoint(L, -1);
  lv_image_set_pivot(obj, point.x, point.y);
  return 1;
}

static const luavgl_property_ops_t img_property_ops[] = {
    {.name = "pivot", .ops = image_set_pivot},
};

static const luavgl_table_t img_property_table = {
    .len = sizeof(img_property_ops) / sizeof(img_property_ops[0]),
    .array = img_property_ops,
};

/**
 * img.set_src(img, "path")
 */
static int luavgl_img_set_src(lua_State *L)
{
  lv_obj_t *obj = luavgl_to_obj(L, 1);

  if (lua_type(L, 2) == LUA_TSTRING) {
    const char *src = luavgl_toimgsrc(L, 2);
    if (src != NULL) {
      lv_image_set_src(obj, src);
    }
    return 0;
  }
  else if (lua_type(L, 2) == LUA_TTABLE) {
    // Allocate new lv_img_dsc_t
    lv_img_dsc_t *dsc = (lv_img_dsc_t *)lv_malloc(sizeof(lv_img_dsc_t));
    if (!dsc) return luaL_error(L, "no memory");

    // fill header fields
    lua_getfield(L, 2, "header");
    if (!lua_istable(L, -1)) return luaL_error(L, "header missing");

    lua_getfield(L, -1, "w");
    dsc->header.w = lua_tointeger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "h");
    dsc->header.h = lua_tointeger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "cf");
    dsc->header.cf = lua_tointeger(L, -1);
    lua_pop(L, 1);

    lua_pop(L, 1); // pop header table

    // get data_size
    lua_getfield(L, 2, "data_size");
    dsc->data_size = lua_tointeger(L, -1);
    lua_pop(L, 1);

    // get data
    // lua_getfield(L, 2, "data");
    // size_t len = 0;
    // const uint8_t *data = (const uint8_t *)lua_tolstring(L, -1, &len);
    // if (data == NULL || len < dsc->data_size) {
    //   return luaL_error(L, "invalid data field");
    // }
    // dsc->data = data;
    // lua_pop(L, 1);

    lua_getfield(L, 2, "data");
    size_t len = 0;
    const uint8_t *src_data = (const uint8_t *)lua_tolstring(L, -1, &len);
    if (src_data == NULL || len < dsc->data_size) {
        return luaL_error(L, "invalid data field");
    }

    // allocate a new buffer
    uint8_t *copy = (uint8_t *)lv_malloc(dsc->data_size);
    if (!copy) return luaL_error(L, "no memory for image data");

    memcpy(copy, src_data, dsc->data_size);
    dsc->data = copy; // now safe
    lua_pop(L, 1);

    // bind it
    lv_image_set_src(obj, dsc);

    // prevent GC of descriptor and pixel data
    lua_pushvalue(L, 2); // push the image descriptor table
    lua_setuservalue(L, 1); // set it as uservalue of the lvgl img object

    return 0;
  }
  else {
    return luaL_error(L, "invalid img src type");
  }
}


/**
 * img:set_offset({x=10})
 * img:set_offset({x=10})
 * img:set_offset({x=10, y=100})
 */
static int luavgl_img_set_offset(lua_State *L)
{
  lv_obj_t *obj = luavgl_to_obj(L, 1);

  if (!lua_istable(L, -1)) {
    luaL_argerror(L, -1, "should be table {x=0,y=0,anim=true}");
  }

  lv_coord_t v;
  lua_getfield(L, -1, "x");
  if (!lua_isnil(L, -1)) {
    v = lua_tointeger(L, -1);
    lua_pop(L, 1);
    lv_image_set_offset_x(obj, v);
  }

  lua_getfield(L, -1, "y");
  if (!lua_isnil(L, -1)) {
    v = lua_tointeger(L, -1);
    lua_pop(L, 1);
    lv_image_set_offset_y(obj, v);
  }

  return 0;
}

/**
 * img:set_pivot({x=10, y=100})
 */
static int luavgl_img_set_pivot(lua_State *L)
{
  lv_obj_t *obj = luavgl_to_obj(L, 1);

  if (!lua_istable(L, -1)) {
    luaL_argerror(L, -1, "should be table {x=0,y=0,anim=true}");
  }

  lv_coord_t x = 0, y = 0;
  lua_getfield(L, -1, "x");
  x = lua_tointeger(L, -1);

  lua_getfield(L, -1, "y");
  y = lua_tointeger(L, -1);

  lv_image_set_pivot(obj, x, y);

  return 0;
}

/**
 * return image size w, h
 * img:get_img_size() -- get size of this image
 * img:get_img_size("src") -- get size of img "src"
 */
static int luavgl_get_img_size(lua_State *L)
{
  lv_obj_t *obj = luavgl_to_obj(L, 1);

  const void *src = NULL;
  if (lua_isnoneornil(L, 2)) {
    src = lv_image_get_src(obj);
  } else {
    src = luavgl_toimgsrc(L, 2);
  }

  lv_image_header_t header;
  if (src == NULL || lv_image_decoder_get_info(src, &header) != LV_RES_OK) {
    lua_pushnil(L);
    lua_pushnil(L);
  } else {
    lua_pushinteger(L, header.w);
    lua_pushinteger(L, header.h);
  }

  return 2;
}

static const rotable_Reg luavgl_img_methods[] = {
    {"set_src",      LUA_TFUNCTION,      {luavgl_img_set_src}        },
    {"set_offset",   LUA_TFUNCTION,      {luavgl_img_set_offset}     },
    {"set_pivot",    LUA_TFUNCTION,      {luavgl_img_set_pivot}      },
    {"get_img_size", LUA_TFUNCTION,      {luavgl_get_img_size}       },
    {"__property",   LUA_TLIGHTUSERDATA, {.ptr = &img_property_table}},
    {0,              0,                  {0}                         },
};

static void luavgl_img_init(lua_State *L)
{
  luavgl_obj_newmetatable(L, &lv_image_class, "lv_img", luavgl_img_methods);
  lua_pop(L, 1);
}
