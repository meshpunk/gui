#include "TouchDrvGT911.hpp"
#include "utilities.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Ticker.h> // Include ticker for LVGL timing
#include <Wire.h>
#include <LittleFS.h>
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

// Data directory paths
#define LUA_PATH "/lua/"
#define SOUNDS_PATH "/sounds/"
#define IMAGES_PATH "/images/"

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

// Filesystem variables
bool fs_mounted = false;

// Helper functions for Lua file loading
String readFile(const char* filename) {
  if (!fs_mounted) {
    Serial.println("Filesystem not mounted!");
    return "";
  }
  
  fs::File file = LittleFS.open(filename, "r");
  if (!file) {
    Serial.print("Failed to open file: ");
    Serial.println(filename);
    return "";
  }
  
  String content = "";
  while (file.available()) {
    content += (char)file.read();
  }
  file.close();
  
  return content;
}

bool loadLuaScript(lua_State *L, const char* filename) {
  String scriptPath = String(LUA_PATH) + filename;
  String script = readFile(scriptPath.c_str());
  
  if (script.length() == 0) {
    Serial.print("Error loading Lua script: ");
    Serial.println(scriptPath);
    return false;
  }
  
  Serial.print("Executing Lua script: ");
  Serial.println(scriptPath);
  
  if (luaL_dostring(L, script.c_str()) != 0) {
    Serial.print("Lua error: ");
    Serial.println(lua_tostring(L, -1));
    lua_pop(L, 1);
    return false;
  }
  
  return true;
}

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
#define LVGL_BUFFER_SIZE (TFT_WIDTH * TFT_HEIGHT * sizeof(lv_color_t))

  static uint8_t *buf = (uint8_t *)ps_malloc(LVGL_BUFFER_SIZE);
  if (!buf) {
    Serial.println("Memory allocation failed!");
    delay(5000);
    assert(buf);
  }

  lv_init();

  // Create a default group for focusable objects
  lv_group_t *default_group = lv_group_create();
  lv_group_set_default(default_group);

  // Create a display
  lv_display_t *disp = lv_display_create(TFT_HEIGHT, TFT_WIDTH);

  // Initialize the buffer
  lv_display_set_buffers(disp, buf, NULL, LVGL_BUFFER_SIZE,
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
  // lv_obj_t *scr = lv_scr_act();

  // Create a label
  // label = lv_label_create(scr);
  // lv_label_set_text(label, "Hello MeshPunk World!");
  // lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 10);

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

  // Open standard Lua libraries
  luaL_openlibs(L);

  // Initialize LuaVGL
  luaL_requiref(L, "lvgl", luaopen_lvgl, 1);
  lua_pop(L, 1);
  
  // Add Lua loader for require function
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "searchers");
  
  // Get the length of the searchers table
  int len = lua_rawlen(L, -1);
  
  // Custom loader function for the filesystem
  lua_pushcfunction(L, [](lua_State *L) -> int {
    const char *modname = luaL_checkstring(L, 1);
    String filename = String(LUA_PATH) + modname + ".lua";
    
    String content = readFile(filename.c_str());
    if (content.length() == 0) {
      lua_pushfstring(L, "\n\tno file '%s' in LittleFS", filename.c_str());
      return 1;  // Return the error message
    }
    
    if (luaL_loadbuffer(L, content.c_str(), content.length(), filename.c_str()) != 0) {
      lua_error(L);
    }
    
    return 1;  // Return the loaded chunk
  });
  
  // Add our loader to the searchers table
  lua_rawseti(L, -2, len + 1);
  lua_pop(L, 2);  // Pop package.searchers and package

  // Setup print function to redirect to Serial
  luaL_dostring(L, R"(
    local old_print = print
    print = function(...)
      local args = {...}
      local text = ""
      for i, v in ipairs(args) do
        text = text .. tostring(v) .. (i < #args and "\t" or "")
      end
      old_print(text)
    end
  )");

  Serial.println("LuaVGL environment initialized");
  
  // Load and run the main script
  if (fs_mounted) {
    if (loadLuaScript(L, "messenger.lua")) {
      Serial.println("Messenger app loaded successfully");
    } else {
      Serial.println("Failed to load messenger app, using fallback");
      
      // Fallback to simple embedded script if the file isn't found
      const char *fallbackScript = R"(
        -- Fallback script when filesystem is not available
        local root = lvgl.Object()
        root:set { w = lvgl.HOR_RES(), h = lvgl.VER_RES() }
        
        root:Label {
          text = "Filesystem Error\nMake sure to upload data files",
          align = lvgl.ALIGN.CENTER
        }
        
        return root
      )";
      
      if (luaL_dostring(L, fallbackScript) != 0) {
        Serial.print("Fallback script error: ");
        Serial.println(lua_tostring(L, -1));
        lua_pop(L, 1);
      }
    }
  } else {
    Serial.println("Filesystem not mounted, can't load Lua scripts");
    
    // Simple fallback UI when no filesystem
    const char *fallbackScript = R"(
      -- Fallback script when filesystem is not available
      local root = lvgl.Object()
      root:set { w = lvgl.HOR_RES(), h = lvgl.VER_RES() }
      
      root:Label {
        text = "Filesystem Error\nMake sure to upload data files",
        align = lvgl.ALIGN.CENTER
      }
      
      return root
    )";
    
    if (luaL_dostring(L, fallbackScript) != 0) {
      Serial.print("Fallback script error: ");
      Serial.println(lua_tostring(L, -1));
      lua_pop(L, 1);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("MeshPunk LuaVGL Demo");
  
  // Initialize filesystem
  if (LittleFS.begin(true)) {
    fs_mounted = true;
    Serial.println("LittleFS mounted successfully");
    
    // List root directory contents
    fs::File root = LittleFS.open("/");
    fs::File file = root.openNextFile();
    
    Serial.println("LittleFS contents:");
    while (file) {
      Serial.print("  ");
      Serial.print(file.name());
      Serial.print(" (");
      Serial.print(file.size());
      Serial.println(" bytes)");
      file = root.openNextFile();
    }
  } else {
    Serial.println("Error mounting LittleFS");
  }

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

  // Adjust backlight
  pinMode(BOARD_BL_PIN, OUTPUT);
  setBrightness(16);
}

void loop() {
  // Handle LVGL tasks
  lv_timer_handler();

  // Check for touch directly from sensor (as additional test)
  static unsigned long last_direct_check = 0;
  if (millis() - last_direct_check > 300) {
    if (touch.isPressed()) {
      uint8_t touched = touch.getPoint(x, y, touch.getSupportTouchPoint());
      if (touched > 0 && touch_debug) {
        // Serial.print("Direct touch check: x=");
        // Serial.print(x[0]);
        // Serial.print(" y=");
        // Serial.println(y[0]);
      }
    }
    last_direct_check = millis();
  }

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

  delay(5);
}