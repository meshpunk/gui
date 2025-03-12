#include "TouchDrvGT911.hpp"
#include "utilities.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Ticker.h> // Include ticker for LVGL timing
#include <Wire.h>
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <luavgl.h>
}

#include <TFT_eSPI.h>
#include <lvgl.h>

// Keyboard I2C defines
#define LILYGO_KB_SLAVE_ADDRESS 0x55
#define LILYGO_KB_BRIGHTNESS_CMD 0x01
#define LILYGO_KB_ALT_B_BRIGHTNESS_CMD 0x02

// Ticker for LVGL timing
Ticker lvgl_ticker;

// LVGL display and touch globals
TFT_eSPI tft;
TouchDrvGT911 touch;

// LuaVGL state
lua_State *L = NULL;

// Keyboard variables
bool keyboard_available = false;
char last_key = 0;

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
  tft.pushColors((uint16_t *)px_map, w * h, true);
  tft.endWrite();
  lv_display_flush_ready(disp);
}

// Touch handling
int16_t x[5], y[5];

// Debug flag for touch
bool touch_debug = true;
unsigned long last_touch_debug = 0;

// Keyboard functions
void setKeyboardBrightness(uint8_t value) {
  if (!keyboard_available)
    return;

  Wire.beginTransmission(LILYGO_KB_SLAVE_ADDRESS);
  Wire.write(LILYGO_KB_BRIGHTNESS_CMD);
  Wire.write(value);
  Wire.endTransmission();
}

void setKeyboardDefaultBrightness(uint8_t value) {
  if (!keyboard_available)
    return;

  Wire.beginTransmission(LILYGO_KB_SLAVE_ADDRESS);
  Wire.write(LILYGO_KB_ALT_B_BRIGHTNESS_CMD);
  Wire.write(value);
  Wire.endTransmission();
}

// Keyboard state tracking variables
static uint32_t last_key_code = 0;
static bool key_is_new = false;
static uint32_t last_key_time = 0;

// LVGL keyboard read callback
static void keyboard_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
  static bool was_pressed = false;
  uint32_t current_time = millis();

  // Read key from keyboard
  char keyValue = 0;
  Wire.requestFrom(LILYGO_KB_SLAVE_ADDRESS, 1);
  if (Wire.available() > 0) {
    keyValue = Wire.read();

    if (keyValue != 0) {
      // Check if this is a new key press or key has been held long enough for
      // repeat
      if (!was_pressed || (last_key_code != keyValue) ||
          (current_time - last_key_time > 30)) {

        last_key_code = keyValue;
        last_key_time = current_time;
        key_is_new = true;
        was_pressed = true;

        // Serial.print("Key registered: ");
        // Serial.print(keyValue);
        // Serial.print(" (");
        // Serial.print((int)keyValue);
        // Serial.println(")");
      }
    } else {
      was_pressed = false;
    }
  }

  // Report key press to LVGL
  if (key_is_new) {
    data->state = LV_INDEV_STATE_PRESSED;
    key_is_new = false;

    // Map special keys
    if (last_key_code == 13) { // Enter
      data->key = LV_KEY_ENTER;
    } else if (last_key_code == 27) { // Escape
      data->key = LV_KEY_ESC;
    } else if (last_key_code == 8) { // Backspace
      data->key = LV_KEY_BACKSPACE;
    } else if (last_key_code == 9) { // Tab
      data->key = LV_KEY_NEXT;
    } else {
      data->key = last_key_code;
    }

    // Serial.print("Sending key to LVGL: ");
    // Serial.println((char)data->key);
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

static void touchpad_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
  data->state = LV_INDEV_STATE_RELEASED;

  if (touch.isPressed()) {
    uint8_t touched = touch.getPoint(x, y, touch.getSupportTouchPoint());
    if (touched > 0) {
      // Print touch coordinates for debugging (limit frequency to avoid
      // flooding serial)
      if (touch_debug && (millis() - last_touch_debug > 500)) {
        // Serial.print("Touch detected! x=");
        // Serial.print(x[0]);
        // Serial.print(" y=");
        // Serial.println(y[0]);
        last_touch_debug = millis();
      }

      data->state = LV_INDEV_STATE_PRESSED;
      data->point.x = x[0];
      data->point.y = y[0];
    }
  }
}

// Setup LVGL
void setupLvgl() {
// #define LVGL_BUFFER_SIZE (TFT_WIDTH * TFT_HEIGHT * sizeof(lv_color_t))
#define LVGL_BUFFER_LINES 240 // Choose a smaller chunk to fit into RAM

  // static uint8_t *buf = (uint8_t *)ps_malloc(LVGL_BUFFER_SIZE);
  // if (!buf) {
  //   Serial.println("Memory allocation failed!");
  //   delay(5000);
  //   assert(buf);
  // }

  lv_init();

  // Create a default group for focusable objects
  lv_group_t *default_group = lv_group_create();
  lv_group_set_default(default_group);

  // Create a display
  lv_display_t *disp = lv_display_create(TFT_HEIGHT, TFT_WIDTH);

  // Initialize the buffer
  static uint8_t *buf1 =
      (uint8_t *)ps_malloc(TFT_WIDTH * LVGL_BUFFER_LINES * sizeof(lv_color_t));
  static uint8_t *buf2 =
      (uint8_t *)ps_malloc(TFT_WIDTH * LVGL_BUFFER_LINES * sizeof(lv_color_t));

  if (!buf1 || !buf2) {
    Serial.println("Memory allocation failed!");
    delay(5000);
    assert(buf1 && buf2);
  }

  lv_display_set_buffers(disp, buf1, buf2,
                         TFT_WIDTH * LVGL_BUFFER_LINES * sizeof(lv_color_t),
                         LV_DISPLAY_RENDER_MODE_FULL);

  // Set display properties
  lv_display_set_flush_cb(disp, disp_flush_cb);
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_0);

  // Register a touchscreen input device
  lv_indev_t *touch_indev = lv_indev_create();
  lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(touch_indev, touchpad_read_cb);
  lv_indev_set_display(touch_indev, disp);

  // Register keyboard input device if available
  if (keyboard_available) {
    lv_indev_t *kb_indev = lv_indev_create();
    lv_indev_set_type(kb_indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(kb_indev, keyboard_read_cb);

    // Connect keyboard to the default group
    lv_indev_set_group(kb_indev, lv_group_get_default());

    Serial.println("Keyboard input device registered with LVGL");
  }
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

  // // Create a button
  // lv_obj_t *btn = lv_btn_create(scr);
  // lv_obj_set_pos(btn, 50, 100);
  // lv_obj_set_size(btn, 120, 50);
  // lv_obj_add_event_cb(btn, btn_event_handler, LV_EVENT_CLICKED, NULL);

  // // Create label on the button
  // lv_obj_t *btn_label = lv_label_create(btn);
  // lv_label_set_text(btn_label, "Click Me!");
  // lv_obj_center(btn_label);
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
        local function createBtn(parent, name)
            local root = parent:Button {
                w = lvgl.SIZE_CONTENT,
                h = lvgl.SIZE_CONTENT,
            }

            root:onClicked(function()
                print("Button clicked!")
                root:set { bg_color = "#00FF00" }
            end)

            root:Label {
                text = name,
                text_color = "#333",
                align = lvgl.ALIGN.CENTER,
            }
        end

        -- Create a simple hello world label
        local root = lvgl.Object()
        root:set { w = lvgl.HOR_RES(), h = lvgl.VER_RES() }

        -- flex layout and align
        root:set {
            flex = {
                flex_direction = "column",
                flex_wrap = "wrap",
                justify_content = "center",
                align_items = "center",
                align_content = "center",
            },
            w = 320,
            h = 240,
            align = lvgl.ALIGN.CENTER
        }

        label = root:Label {
            text = string.format("Hello %03d", 123),
            text_font = lvgl.BUILTIN_FONT.MONTSERRAT_28,
            align = lvgl.ALIGN.CENTER
        }        

        createBtn(root, "Button")

        -- Create textarea
        local ta = root:Textarea {
            password_mode = false,
            one_line = true,
            text = "Input text here",
            w = lvgl.SIZE_CONTENT,
            h = lvgl.SIZE_CONTENT,
            pad_all = 2,
            align = lvgl.ALIGN.TOP_MID,
        }

        print("created textarea: ", ta)

        -- second anim example with playback
        local obj = parent:Object {
            bg_color = "#F00000",
            radius = lvgl.RADIUS_CIRCLE,
            size = 64,
            x = 64,
            y = 64
        }
        obj:clear_flag(lvgl.FLAG.SCROLLABLE)

        --- @type AnimPara
        local animPara = {
            run = true,
            start_value = 16,
            end_value = 32,
            duration = 1000,
            repeat_count = lvgl.ANIM_REPEAT_INFINITE,
            path = "ease_in_out",
        }

        animPara.exec_cb = function(obj, value)
            obj:set { size = value }
        end

        obj:Anim(animPara)

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

  // Initialize keyboard
  Wire.beginTransmission(LILYGO_KB_SLAVE_ADDRESS);
  if (Wire.endTransmission() == 0) {
    keyboard_available = true;
    Serial.println("T-Deck keyboard found!");

    // Set initial keyboard brightness
    setKeyboardDefaultBrightness(127);
    setKeyboardBrightness(200);
  } else {
    Serial.println("T-Deck keyboard not found!");
  }

  // LVGL tick function
  lvgl_ticker.attach_ms(5, []() {
    lv_tick_inc(5); // Increment LVGL tick counter every 5ms
  });

  // Initialize LVGL
  setupLvgl();

  // Initialize LuaVGL
  setupLuaVGL();

  // Create UI
  createUI();

  // Create a test textarea for keyboard input
  if (keyboard_available) {
    // Get the active screen
    lv_obj_t *scr = lv_scr_act();

    // Create a label for instructions
    lv_obj_t *instructions = lv_label_create(scr);
    lv_label_set_text(instructions, "Testing keyboard input:");
    lv_obj_align(instructions, LV_ALIGN_TOP_MID, 0, 50);

    // Create a text area for keyboard input testing
    lv_obj_t *ta = lv_textarea_create(scr);
    lv_obj_set_size(ta, 280, 60);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 80);
    lv_textarea_set_placeholder_text(ta, "Type with keyboard...");
    lv_obj_add_state(ta, LV_STATE_FOCUSED); // Give it initial focus

    // Add textarea to keyboard group
    lv_group_t *g = lv_group_get_default();
    if (g) {
      lv_group_add_obj(g, ta);

      // Set it as the default focused object
      lv_group_focus_obj(ta);

      // Serial.println("Added textarea to keyboard group with focus");
    }
  }

  // Adjust backlight
  pinMode(BOARD_BL_PIN, OUTPUT);
  setBrightness(16);
}

void loop() {
  // Handle LVGL tasks
  lv_timer_handler();

  // Check for touch directly from sensor (as additional test)
  // static unsigned long last_direct_check = 0;
  // if (millis() - last_direct_check > 300) {
  //   if (touch.isPressed()) {
  //     uint8_t touched = touch.getPoint(x, y, touch.getSupportTouchPoint());
  //     if (touched > 0 && touch_debug) {
  //       // Serial.print("Direct touch check: x=");
  //       // Serial.print(x[0]);
  //       // Serial.print(" y=");
  //       // Serial.println(y[0]);
  //     }
  //   }
  //   last_direct_check = millis();
  // }

  // Check keyboard directly (useful for debugging)
  // static unsigned long last_kb_check = 0;
  // if (keyboard_available && millis() - last_kb_check > 100) {
  //   char keyValue = 0;
  //   Wire.requestFrom(LILYGO_KB_SLAVE_ADDRESS, 1);
  //   if (Wire.available() > 0) {
  //     keyValue = Wire.read();
  //     if (keyValue != 0) {
  //       // Serial.print("Direct keyboard check: key=");
  //       // Serial.print(keyValue);
  //       // Serial.print(" (");
  //       // Serial.print((int)keyValue);
  //       // Serial.println(")");
  //     }
  //   }
  //   last_kb_check = millis();
  // }

  vTaskDelay(pdMS_TO_TICKS(1)); // Minimize blocking

  // delay(1);
}