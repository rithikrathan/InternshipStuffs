#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <Wire.h>
#include <TAMC_GT911.h>

/* --- HARDWARE CONFIG --- */
#define I2C_SDA 8
#define I2C_SCL 9
#define TP_INT 4
#define TP_RST -1 // Let CH422G handle reset

// Standard Waveshare 7" RGB Bus Config
Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
  5, 3, 46, 7, 1, 2, 42, 41, 40, 39, 0, 45, 48, 47, 21, 14, 38, 18, 17, 10,
  1, 40, 48, 40, 1, 13, 3, 31, 1, 16000000);
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(800, 480, bus);

// Pass -1 for reset pin
TAMC_GT911 ts = TAMC_GT911(I2C_SDA, I2C_SCL, TP_INT, TP_RST, 800, 480);

/* --- LVGL CALLBACKS --- */
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint32_t w = lv_area_get_width(area);
  uint32_t h = lv_area_get_height(area);
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);
  lv_display_flush_ready(disp);
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
  delay(1);
  ts.read();

  if (ts.isTouched) {
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = ts.points[0].x;
    data->point.y = ts.points[0].y;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void setup() {
  Serial.begin(115200);

  Wire.begin(I2C_SDA, I2C_SCL);
  pinMode(TP_INT, INPUT);

  // --- IO EXPANDER RESET (CH422G for Waveshare 7-inch) ---
  Wire.beginTransmission(0x24);
  Wire.write(0x01); 
  Wire.endTransmission();

  Wire.beginTransmission(0x38);
  Wire.write(0x00); 
  Wire.endTransmission();
  delay(20); 

  Wire.beginTransmission(0x38);
  Wire.write(0x1E); 
  Wire.endTransmission();
  delay(200); 

  // --- OPTIONAL: fallback address check ---
  Wire.beginTransmission(0x5D);
  if (Wire.endTransmission() != 0) {
    Serial.println("GT911 not at 0x5D, trying 0x14");
    ts = TAMC_GT911(I2C_SDA, I2C_SCL, TP_INT, TP_RST, 800, 480);
  }

  if (!gfx->begin()) {
    Serial.println("GFX Init Failed!");
    return;
  }

  ts.begin();
  ts.setRotation(ROTATION_NORMAL);

  lv_init();

  lv_display_t *disp = lv_display_create(800, 480);
  lv_display_set_flush_cb(disp, my_disp_flush);

  static uint8_t *buf = (uint8_t *)malloc(800 * 60 * 2);
  lv_display_set_buffers(disp, buf, NULL, 800 * 60 * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);

  // --- GUI ---
  lv_obj_t *screen = lv_screen_active();
  lv_obj_set_style_bg_color(screen, lv_palette_main(LV_PALETTE_GREY), 0);

  lv_obj_t *main_cont = lv_obj_create(screen);
  lv_obj_set_size(main_cont, 760, 400);
  lv_obj_center(main_cont);
  lv_obj_set_style_radius(main_cont, 15, 0);
  lv_obj_set_style_border_width(main_cont, 0, 0);

  static int32_t col_dsc[] = { 160, 160, 160, 160, LV_GRID_TEMPLATE_LAST };
  static int32_t row_dsc[] = { 150, 150, LV_GRID_TEMPLATE_LAST };
  lv_obj_set_grid_dsc_array(main_cont, col_dsc, row_dsc);

  for (int i = 0; i < 8; i++) {
    int col = i % 4;
    int row = i / 4;

    lv_obj_t *obj = lv_obj_create(main_cont);
    lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *sw = lv_switch_create(obj);
    lv_obj_align(sw, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t *lbl = lv_label_create(obj);
    lv_label_set_text_fmt(lbl, "SWITCH %d", i + 1);
    lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
  }
}

void loop() {
  lv_timer_handler();
  delay(5);
}
