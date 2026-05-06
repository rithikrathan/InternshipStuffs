#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <Wire.h>
#include <TAMC_GT911.h>

#define I2C_SDA 8
#define I2C_SCL 9
#define TP_INT 4

Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
  5, 3, 46, 7, 1, 2, 42, 41, 40, 39, 0, 45, 48, 47, 21, 14, 38, 18, 17, 10,
  1, 40, 48, 40, 1, 13, 3, 31, 1, 16000000
);
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(800, 480, bus);

TAMC_GT911 ts = TAMC_GT911(I2C_SDA, I2C_SCL, 0x5D, TP_INT, 800, 480);

static lv_display_t *disp;
static lv_obj_t *draw_area;
static lv_obj_t *coord_label;
static lv_obj_t *touch_count_label;
static lv_obj_t *touch_dots[5];
static bool dots_created = false;

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint32_t w = lv_area_get_width(area);
  uint32_t h = lv_area_get_height(area);
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);
  lv_display_flush_ready(disp);
}

void update_coordinates(int16_t x, int16_t y) {
  lv_label_set_text_fmt(coord_label, "X: %d   Y: %d", x, y);
}

void update_touch_count(uint32_t count) {
  lv_label_set_text_fmt(touch_count_label, "Touches: %d", count);
}

void create_touch_dots(lv_obj_t *parent) {
  for (int i = 0; i < 5; i++) {
    touch_dots[i] = lv_obj_create(parent);
    lv_obj_set_size(touch_dots[i], 24, 24);
    lv_obj_set_style_radius(touch_dots[i], 12, 0);
    lv_obj_set_style_bg_color(touch_dots[i], lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_border_width(touch_dots[i], 2, 0);
    lv_obj_set_style_border_color(touch_dots[i], lv_color_white(), 0);
    lv_obj_set_style_opa(touch_dots[i], LV_OPA_80, 0);
    lv_obj_add_flag(touch_dots[i], LV_OBJ_FLAG_HIDDEN);
  }
  dots_created = true;
}

void clear_touch_dots() {
  for (int i = 0; i < 5; i++) {
    lv_obj_add_flag(touch_dots[i], LV_OBJ_FLAG_HIDDEN);
  }
}

void clear_all() {
  clear_touch_dots();
  lv_label_set_text(coord_label, "X: ---   Y: ---");
  lv_label_set_text(touch_count_label, "Touches: 0");
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
  delay(1);
  ts.read();

  if (ts.isTouched) {
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = ts.points[0].x;
    data->point.y = ts.points[0].y;
    update_coordinates(ts.points[0].x, ts.points[0].y);
    update_touch_count(ts.touches);

    if (!dots_created) {
      create_touch_dots(draw_area);
    }

    for (uint8_t i = 0; i < ts.touches && i < 5; i++) {
      lv_obj_remove_flag(touch_dots[i], LV_OBJ_FLAG_HIDDEN);
      lv_obj_align(touch_dots[i], LV_ALIGN_TOP_LEFT, ts.points[i].x - 12, ts.points[i].y - 12);

      lv_palette_t colors[] = {LV_PALETTE_RED, LV_PALETTE_GREEN, LV_PALETTE_BLUE, LV_PALETTE_YELLOW, LV_PALETTE_PURPLE};
      lv_obj_set_style_bg_color(touch_dots[i], lv_palette_main(colors[i]), 0);
    }

    for (uint8_t i = ts.touches; i < 5; i++) {
      lv_obj_add_flag(touch_dots[i], LV_OBJ_FLAG_HIDDEN);
    }
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
    data->point.x = 0;
    data->point.y = 0;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  Wire.begin(I2C_SDA, I2C_SCL);
  pinMode(TP_INT, INPUT);

  auto writeXLExpander = [](uint8_t reg, uint8_t val) {
    Wire.beginTransmission(0x24);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
  };

  writeXLExpander(0x06, 0x00);
  writeXLExpander(0x02, 0xFB);
  delay(20);
  writeXLExpander(0x02, 0xFF);
  delay(200);
  Serial.println("IO Expander done");

  Wire.beginTransmission(0x5D);
  if (Wire.endTransmission() != 0) {
    Serial.println("GT911 not at 0x5D, trying 0x14");
    ts = TAMC_GT911(I2C_SDA, I2C_SCL, 0x14, TP_INT, 800, 480);
  } else {
    Serial.println("GT911 found at 0x5D");
  }

  if (!gfx->begin()) {
    Serial.println("GFX begin failed!");
    return;
  }
  Serial.println("GFX begin OK");

  ts.begin();
  ts.setRotation(ROTATION_NORMAL);
  Serial.println("Touch begin OK");

  lv_init();
  Serial.println("LVGL init OK");

  disp = lv_display_create(800, 480);
  lv_display_set_flush_cb(disp, my_disp_flush);

  static uint8_t *buf = (uint8_t *)malloc(800 * 60 * 2);
  lv_display_set_buffers(disp, buf, NULL, 800 * 60 * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);
  Serial.println("Display buffer OK");

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);
  Serial.println("Input device OK");

  lv_obj_t *screen = lv_screen_active();
  lv_obj_set_style_bg_color(screen, lv_palette_main(LV_PALETTE_GREY), 0);

  lv_obj_t *header = lv_label_create(screen);
  lv_label_set_text(header, "Touch Test");
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 20);

  lv_obj_t *info_cont = lv_obj_create(screen);
  lv_obj_set_size(info_cont, 300, 80);
  lv_obj_align(info_cont, LV_ALIGN_TOP_MID, 0, 70);
  lv_obj_set_style_bg_color(info_cont, lv_palette_main(LV_PALETTE_BLUE_GREY), 0);
  lv_obj_set_style_radius(info_cont, 10, 0);
  lv_obj_set_style_border_width(info_cont, 0, 0);

  coord_label = lv_label_create(info_cont);
  lv_label_set_text(coord_label, "X: ---   Y: ---");
  lv_obj_set_style_text_color(coord_label, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_align(coord_label, LV_ALIGN_TOP_MID, 0, 10);

  touch_count_label = lv_label_create(info_cont);
  lv_label_set_text(touch_count_label, "Touches: 0");
  lv_obj_set_style_text_color(touch_count_label, lv_palette_main(LV_PALETTE_GREY), 0);
  lv_obj_align(touch_count_label, LV_ALIGN_BOTTOM_MID, 0, -10);

  lv_obj_t *clear_btn = lv_btn_create(screen);
  lv_obj_set_size(clear_btn, 100, 45);
  lv_obj_align(clear_btn, LV_ALIGN_TOP_RIGHT, -20, 20);
  lv_obj_set_style_bg_color(clear_btn, lv_palette_main(LV_PALETTE_RED), 0);

  lv_obj_t *clear_btn_label = lv_label_create(clear_btn);
  lv_label_set_text(clear_btn_label, "Clear");
  lv_obj_align(clear_btn_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_event_cb(clear_btn, [](lv_event_t *e) {
    clear_all();
  }, LV_EVENT_CLICKED, NULL);

  draw_area = lv_obj_create(screen);
  lv_obj_set_size(draw_area, 780, 325);
  lv_obj_align(draw_area, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_set_style_bg_color(draw_area, lv_palette_main(LV_PALETTE_GREY), 0);
  lv_obj_set_style_radius(draw_area, 10, 0);
  lv_obj_set_style_border_width(draw_area, 2, 0);
  lv_obj_set_style_border_color(draw_area, lv_palette_main(LV_PALETTE_BLUE_GREY), 0);

  lv_obj_t *hint = lv_label_create(screen);
  lv_label_set_text(hint, "Draw on the canvas above");
  lv_obj_set_style_text_color(hint, lv_palette_main(LV_PALETTE_GREY), 0);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -5);

  Serial.println("Setup complete!");
}

void loop() {
  lv_timer_handler();
  delay(5);
}
