#include "TouchDrvGT911.hpp"
#include "utilities.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <luavgl.h>
}

#include <TFT_eSPI.h>
#include <lvgl.h>

// LVGL display and touch globals
TFT_eSPI tft;
TouchDrvGT911 touch;

// LuaVGL state
lua_State *L = NULL;

// LilyGo T-Deck control backlight chip has 16 levels of adjustment range
// The adjustable range is 0~15, 0 is the minimum brightness, 15 is the maximum
// brightness
void setBrightness(uint8_t value) {
  static uint8_t level = 0;
  static uint8_t steps = 16;
  if (value == 0) {
    digitalWrite(BOARD_BL_PIN, 0);
    delay(3);
    level = 0;
    return;
  }
  if (level == 0) {
    digitalWrite(BOARD_BL_PIN, 1);
    level = steps;
    delayMicroseconds(30);
  }
  int from = steps - level;
  int to = steps - value;
  int num = (steps + to - from) % steps;
  for (int i = 0; i < num; i++) {
    digitalWrite(BOARD_BL_PIN, 0);
    digitalWrite(BOARD_BL_PIN, 1);
  }
  level = value;
}

// LVGL display driver
static void disp_flush_cb(lv_display_t *disp, const lv_area_t *area,
                          uint8_t *px_map) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)px_map, w * h, false);
  tft.endWrite();
  lv_display_flush_ready(disp);
}

// Touch handling
int16_t x[5], y[5];

static void touchpad_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
  data->state = LV_INDEV_STATE_RELEASED;

  if (touch.isPressed()) {
    uint8_t touched = touch.getPoint(x, y, touch.getSupportTouchPoint());
    if (touched > 0) {
      data->state = LV_INDEV_STATE_PRESSED;
      data->point.x = x[0];
      data->point.y = y[0];
    }
  }
}

// Setup LVGL
void setupLvgl() {
#define LVGL_BUFFER_SIZE (TFT_WIDTH * TFT_HEIGHT * sizeof(lv_color_t))

  static uint8_t *buf = (uint8_t *)ps_malloc(LVGL_BUFFER_SIZE);
  if (!buf) {
    Serial.println("Memory allocation failed!");
    delay(5000);
    assert(buf);
  }

  lv_init();

  lv_group_set_default(lv_group_create());

  // Create a display
  lv_display_t *disp = lv_display_create(TFT_HEIGHT, TFT_WIDTH);

  // Initialize the buffer
  lv_display_set_buffers(disp, buf, NULL, LVGL_BUFFER_SIZE,
                         LV_DISPLAY_RENDER_MODE_FULL);

  // Set display properties
  lv_display_set_flush_cb(disp, disp_flush_cb);
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_0);

  // Register a touchscreen input device
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touchpad_read_cb);
  lv_indev_set_display(indev, disp);
}

// LVGL UI elements
static lv_obj_t *label;

// Event handler for button
static void btn_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    lv_label_set_text(label, "Button was clicked!");
  }
}

// Create a simple UI
void createUI() {
  // Get the active screen
  lv_obj_t *scr = lv_scr_act();

  // Create a label
  label = lv_label_create(scr);
  lv_label_set_text(label, "Hello MeshPunk World!");
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 10);

  // Create a button
  lv_obj_t *btn = lv_btn_create(scr);
  lv_obj_set_pos(btn, 50, 100);
  lv_obj_set_size(btn, 120, 50);
  lv_obj_add_event_cb(btn, btn_event_handler, LV_EVENT_CLICKED, NULL);

  // Create label on the button
  lv_obj_t *btn_label = lv_label_create(btn);
  lv_label_set_text(btn_label, "Click Me!");
  lv_obj_center(btn_label);
}

// Initialize LuaVGL
void setupLuaVGL() {
  // Create Lua state
  L = luaL_newstate();
  if (!L) {
    Serial.println("Failed to create Lua state");
    return;
  }

  // lv_obj_t *root = lv_obj_create(lv_scr_act());
  // lv_obj_set_size(root, LV_PCT(TFT_HEIGHT), LV_PCT(TFT_WIDTH));
  // lv_obj_set_style_outline_width(root, 2, 0);
  // lv_obj_set_style_bg_color(root, lv_color_hex(0xff00ff), 0);
  // lv_obj_set_style_bg_opa(root, LV_OPA_50, 0);

  // Open standard Lua libraries
  luaL_openlibs(L);

  // Initialize LuaVGL
  luaL_requiref(L, "lvgl", luaopen_lvgl, 1);
  lua_pop(L, 1);

  // Load and run LuaVGL hello world script
  const char *luaScript = R"(
        -- Create a simple hello world label
        local root = lvgl.Object()
        root:set { w = lvgl.HOR_RES(), h = lvgl.VER_RES() }

        -- flex layout and align
        root:set {
            flex = {
                flex_direction = "row",
                flex_wrap = "wrap",
                justify_content = "center",
                align_items = "center",
                align_content = "center",
            },
            w = 300,
            h = 75,
            align = lvgl.ALIGN.CENTER
        }

        -- create obj on root
        local obj = root:Object()

    )";

  if (luaL_dostring(L, luaScript) != 0) {
    Serial.print("LuaVGL script error: ");
    Serial.println(lua_tostring(L, -1));
    lua_pop(L, 1);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("MeshPunk LuaVGL Demo");

  // The board peripheral power control pin needs to be set to HIGH when using
  // the peripheral
  pinMode(BOARD_POWERON, OUTPUT);
  digitalWrite(BOARD_POWERON, HIGH);

  // Set CS on all SPI buses to high level during initialization
  pinMode(BOARD_SDCARD_CS, OUTPUT);
  pinMode(RADIO_CS_PIN, OUTPUT);
  pinMode(BOARD_TFT_CS, OUTPUT);

  digitalWrite(BOARD_SDCARD_CS, HIGH);
  digitalWrite(RADIO_CS_PIN, HIGH);
  digitalWrite(BOARD_TFT_CS, HIGH);

  pinMode(BOARD_SPI_MISO, INPUT_PULLUP);
  SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI); // SD

  pinMode(BOARD_BOOT_PIN, INPUT_PULLUP);
  pinMode(BOARD_TBOX_G02, INPUT_PULLUP);
  pinMode(BOARD_TBOX_G01, INPUT_PULLUP);
  pinMode(BOARD_TBOX_G04, INPUT_PULLUP);
  pinMode(BOARD_TBOX_G03, INPUT_PULLUP);

  Serial.println("Initializing display");
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // Set touch int input
  pinMode(BOARD_TOUCH_INT, INPUT);
  delay(20);

  Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);

  touch.setPins(-1, BOARD_TOUCH_INT);
  if (!touch.begin(Wire, GT911_SLAVE_ADDRESS_L)) {
    while (1) {
      Serial.println("Failed to find GT911 - check your wiring!");
      delay(1000);
    }
  }

  Serial.println("Init GT911 Sensor success!");

  // Set touch max xy
  touch.setMaxCoordinates(320, 240);

  // Set swap xy
  touch.setSwapXY(true);

  // Set mirror xy
  touch.setMirrorXY(false, true);

  // Initialize LVGL
  setupLvgl();

  // Initialize LuaVGL
  setupLuaVGL();

  // Create UI
  createUI();

  // Adjust backlight
  pinMode(BOARD_BL_PIN, OUTPUT);
  setBrightness(16);
}

void loop() {
  // Handle LVGL tasks
  lv_timer_handler();

  // LuaVGL periodic handler (if needed)
  // if (L) {
  //   luavgl_handle_task(L);
  // }

  delay(5);
}