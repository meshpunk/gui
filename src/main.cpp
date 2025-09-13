#include "TouchDrvGT911.hpp"
#include "utilities.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <TFT_eSPI.h>
#include <Ticker.h> // Include ticker for LVGL timing
#include <WiFi.h>
#include <Wire.h>
#include "tdeck-pins.h"

// WiFi credentials
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <luavgl.h>

int luaL_loadfilex(lua_State *L, const char *filename, const char *mode) {
    File file = LittleFS.open(filename, "r");
    if (!file || file.isDirectory()) {
      lua_pushfstring(L, "cannot open %s", filename);
      return LUA_ERRFILE;
    }

    size_t size = file.size();
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
      file.close();
      lua_pushliteral(L, "out of memory");
      return LUA_ERRMEM;
    }

    file.readBytes(buffer, size);
    buffer[size] = '\0';
    file.close();

    int status = luaL_loadbufferx(L, buffer, size, filename, mode);
    free(buffer);
    return status;
  }

}

#include <TFT_eSPI.h>
#include <lvgl.h>

// Home button
volatile bool homePressed = false;

void IRAM_ATTR ISR_click() {
  homePressed = true;
}

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

// List dir helper
void listDir(fs::FS &fs, const char *dirname, int level = 0) {
  File root = fs.open(dirname);
  if (!root || !root.isDirectory()) {
    Serial.print("Failed to open directory: ");
    Serial.println(dirname);
    return;
  }

  File file = root.openNextFile();
  while (file) {
    for (int i = 0; i < level; i++) Serial.print("  ");
    Serial.print(dirname);
    Serial.print("/");
    Serial.print(file.name());
    Serial.print(":");
    Serial.print(file.size());
    Serial.println("b");

    if (file.isDirectory()) {
      String path = String(dirname);
      if (!path.endsWith("/")) path += "/";
      path += file.name();
      listDir(fs, path.c_str(), level + 1);
    }

    file = root.openNextFile();
  }
}

// Helper functions for Lua file loading
String readFile(const char *filename) {
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

static int lua_io_open(lua_State *L) {
  const char *filename = luaL_checkstring(L, 1);
  const char *mode = luaL_optstring(L, 2, "r");

  Serial.print("io.open: ");
  Serial.println(filename);

  if (String(mode) != "r") {
    lua_pushnil(L);
    lua_pushstring(L, "Only 'r' mode supported");
    return 2;
  }

  fs::File f = LittleFS.open(filename, "r");
  if (!f || f.isDirectory()) {
    lua_pushnil(L);
    lua_pushstring(L, "File not found or is a directory");
    return 2;
  }

  // Wrap file in userdata
  fs::File *file = new fs::File(f);
  fs::File **ud = (fs::File **)lua_newuserdata(L, sizeof(fs::File *));
  *ud = file;

  luaL_getmetatable(L, "esp32_file");
  lua_setmetatable(L, -2);
  return 1;
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

// WiFi function for Lua
static int lua_wifi_connect(lua_State *L) {
  const char *network = luaL_checkstring(L, 1);
  const char *pass = luaL_checkstring(L, 2);

  Serial.print("Connecting to WiFi: ");
  Serial.println(network);

  WiFi.begin(network, pass);

  return 0;
}

// WiFi status function for Lua
static int lua_wifi_status(lua_State *L) {
  wl_status_t status = WiFi.status();
  const char *status_str = "unknown";

  switch (status) {
  case WL_CONNECTED:
    status_str = "connected";
    break;
  case WL_IDLE_STATUS:
    status_str = "idle";
    break;
  case WL_DISCONNECTED:
    status_str = "disconnected";
    break;
  case WL_CONNECT_FAILED:
    status_str = "failed";
    break;
  case WL_CONNECTION_LOST:
    status_str = "lost";
    break;
  case WL_NO_SSID_AVAIL:
    status_str = "no_ssid";
    break;
  default:
    status_str = "unknown";
    break;
  }

  lua_pushstring(L, status_str);
  if (status == WL_CONNECTED) {
    lua_pushstring(L, WiFi.localIP().toString().c_str());
    lua_pushstring(L, WiFi.SSID().c_str());
  } else {
    lua_pushstring(L, "");
    lua_pushstring(L, "");
  }

  return 3; // Return status, IP, and SSID
}

// WiFi disconnect function for Lua
static int lua_wifi_disconnect(lua_State *L) {
  WiFi.disconnect();
  return 0;
}

// HTTP fetch function for Lua
static int lua_wifi_fetch(lua_State *L) {
  const char *url = luaL_checkstring(L, 1);
  const char *method = luaL_optstring(L, 2, "GET");

  // Parse headers if provided (table)
  lua_newtable(L); // Create result table

  if (WiFi.status() != WL_CONNECTED) {
    lua_pushboolean(L, 0); // success = false
    lua_setfield(L, -2, "success");

    lua_pushstring(L, "WiFi not connected");
    lua_setfield(L, -2, "error");

    return 1;
  }

  HTTPClient http;
  http.begin(url);

  // Add headers if available (3rd parameter is a table)
  if (!lua_isnoneornil(L, 3) && lua_istable(L, 3)) {
    lua_pushnil(L); // First key
    while (lua_next(L, 3) != 0) {
      // Key at -2, value at -1
      if (lua_isstring(L, -2) && lua_isstring(L, -1)) {
        const char *headerName = lua_tostring(L, -2);
        const char *headerValue = lua_tostring(L, -1);
        http.addHeader(headerName, headerValue);
      }
      lua_pop(L, 1); // Remove value, keep key for next iteration
    }
  }

  int httpCode = 0;
  String payload = "";

  if (strcmp(method, "GET") == 0) {
    httpCode = http.GET();
  } else if (strcmp(method, "POST") == 0) {
    const char *body = luaL_optstring(L, 4, "");
    httpCode = http.POST(body);
  } else if (strcmp(method, "PUT") == 0) {
    const char *body = luaL_optstring(L, 4, "");
    httpCode = http.PUT(body);
  } else if (strcmp(method, "DELETE") == 0) {
    httpCode = http.sendRequest("DELETE");
  } else {
    // Unknown method
    lua_pushboolean(L, 0); // success = false
    lua_setfield(L, -2, "success");

    lua_pushstring(L, "Unsupported HTTP method");
    lua_setfield(L, -2, "error");

    http.end();
    return 1;
  }

  if (httpCode > 0) {
    // HTTP header has been sent and server response header has been handled
    payload = http.getString();

    lua_pushboolean(L, 1); // success = true
    lua_setfield(L, -2, "success");

    lua_pushinteger(L, httpCode);
    lua_setfield(L, -2, "status");

    lua_pushstring(L, payload.c_str());
    lua_setfield(L, -2, "body");
  } else {
    lua_pushboolean(L, 0); // success = false
    lua_setfield(L, -2, "success");

    lua_pushstring(L, http.errorToString(httpCode).c_str());
    lua_setfield(L, -2, "error");
  }

  http.end();
  return 1; // Return the result table
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

  // Register WiFi functions
  lua_register(L, "_wifi_connect", lua_wifi_connect);
  lua_register(L, "_wifi_status", lua_wifi_status);
  lua_register(L, "_wifi_disconnect", lua_wifi_disconnect);
  lua_register(L, "_wifi_fetch", lua_wifi_fetch);

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
      return 1; // Return the error message
    }

    if (luaL_loadbuffer(L, content.c_str(), content.length(),
                        filename.c_str()) != 0) {
      lua_error(L);
    }

    return 1; // Return the loaded chunk
  });

  // Add our loader to the searchers table
  lua_rawseti(L, -2, len + 1);
  lua_pop(L, 2); // Pop package.searchers and package

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

  // Register file metatable
  luaL_newmetatable(L, "esp32_file");

  // Create a method table for the file object
  lua_newtable(L);

  // file:read()
  lua_pushcfunction(L, [](lua_State *L) -> int {
    fs::File **ud = (fs::File **)luaL_checkudata(L, 1, "esp32_file");
    String content = (*ud)->readString(); // Read whole file
    lua_pushstring(L, content.c_str());
    return 1;
  });
  lua_setfield(L, -2, "read");

  // file:close()
  lua_pushcfunction(L, [](lua_State *L) -> int {
    fs::File **ud = (fs::File **)luaL_checkudata(L, 1, "esp32_file");
    (*ud)->close();
    delete *ud;
    *ud = nullptr;
    return 0;
  });
  lua_setfield(L, -2, "close");

  // Set the __index = method table
  lua_setfield(L, -2, "__index");

  // Optional: __gc finalizer (cleanup on Lua garbage collection)
  lua_pushcfunction(L, [](lua_State *L) -> int {
    fs::File **ud = (fs::File **)luaL_checkudata(L, 1, "esp32_file");
    if (*ud) {
      (*ud)->close();
      delete *ud;
      *ud = nullptr;
    }
    return 0;
  });
  lua_setfield(L, -2, "__gc");

  lua_pop(L, 1); // pop metatable

  Serial.println("Added esp32_file");


  // Inject our C++-backed io.open into the Lua global 'io' table
  lua_getglobal(L, "io"); // push io table

  if (lua_isnil(L, -1)) {
    lua_newtable(L);           // create io table if not present
    lua_setglobal(L, "io");    // set it
    lua_getglobal(L, "io");    // push it again
  }

  lua_pushcfunction(L, lua_io_open);
  lua_setfield(L, -2, "open"); // io.open = lua_io_open

  lua_pop(L, 1); // pop io table

  Serial.println("Patched IO");

  Serial.println("LuaVGL environment initialized");

  if (!fs_mounted) {
    Serial.println("Filesystem not mounted, can't load Lua scripts");

    const char *fallbackScript = R"(
    local root = lvgl.Object()
    root:set { w = lvgl.HOR_RES(), h = lvgl.VER_RES() }

    root:Label {
      text = "Filesystem not mounted\nUpload Lua scripts to flash",
      align = lvgl.ALIGN.CENTER
    }

    return root
  )";

    if (luaL_dostring(L, fallbackScript) != 0) {
      Serial.print("Lua fallback script error: ");
      Serial.println(lua_tostring(L, -1));
      lua_pop(L, 1);
    }

    return;
  }

  String scriptPath = String(LUA_PATH) + "main.lua";
  String script = readFile(scriptPath.c_str());

  if (script.length() == 0) {
    Serial.print("Lua script not found: ");
    Serial.println(scriptPath);

    const char *fallbackScript = R"(
    local root = lvgl.Object()
    root:set { w = lvgl.HOR_RES(), h = lvgl.VER_RES() }

    root:Label {
      text = "Lua script missing",
      align = lvgl.ALIGN.CENTER
    }

    return root
  )";

    luaL_dostring(L, fallbackScript); // no need to recheck error here
    return;
  }

  Serial.print("Executing Lua script: ");
  Serial.println(scriptPath);

  if (luaL_dostring(L, script.c_str()) != 0) {
    const char *luaError = lua_tostring(L, -1);
    Serial.print("Lua execution error: ");
    Serial.println(luaError);

    // Escape any embedded quotes or newlines
    String escapedError = String(luaError);
    escapedError.replace("\\", "\\\\");
    escapedError.replace("\"", "\\\"");
    escapedError.replace("\n", "\\n");

    String fallbackScript = R"(
    local root = lvgl.Object()
    root:set { w = lvgl.HOR_RES(), h = lvgl.VER_RES() }

    root:Label {
      text = ")" + escapedError +
                            R"(",
      align = lvgl.ALIGN.CENTER
    }

    return root
  )";

    if (luaL_dostring(L, fallbackScript.c_str()) != 0) {
      Serial.print("Fallback display error: ");
      Serial.println(lua_tostring(L, -1));
    }

    lua_pop(L, 1);
    return;
  }

  return;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Delaying for 2500ms...");
  delay(2500);
  
  Serial.println("MeshPunk LuaVGL Demo");

  // Connect trackball / home button
  pinMode(TDECK_TRACKBALL_CLICK, INPUT_PULLUP);
  attachInterrupt(TDECK_TRACKBALL_CLICK, ISR_click, FALLING);

  // Initialize filesystem
  if (LittleFS.begin(true)) {
    fs_mounted = true;
    Serial.println("LittleFS mounted successfully");

    Serial.println("LittleFS contents:");
    listDir(LittleFS, "/lua");
  } else {
    Serial.println("Error mounting LittleFS!!");
  }

  // Initialize WiFi in station mode
  WiFi.mode(WIFI_STA);
  Serial.println("WiFi initialized in station mode");

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

  // Check for home button press
  if (homePressed) {
    homePressed = false;

    Serial.println("[Home Button] dofile('/launcher')");

    if (L) {
      String scriptPath = String(LUA_PATH) + "main.lua";
      int err = luaL_dofile(L, scriptPath.c_str());
      
      if (err != 0) {
        const char* err_msg = lua_tostring(L, -1);
        Serial.printf("Lua Error: %s\n", err_msg);
        lua_pop(L, 1); // remove error message
      }
      
    } else {
      Serial.println("Lua state is NULL!");
    }
  }

  delay(5);
}