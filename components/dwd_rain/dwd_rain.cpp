#include "dwd_rain.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace esphome::dwd_rain {

using tessera::Font;
namespace ui = tessera::ui;

static constexpr int MAX_BARS = 24;       // two hours in five-minute steps
static constexpr uint32_t RAINY = 0xF0597;    // weather-rainy
static constexpr uint32_t POURING = 0xF0596;  // weather-pouring

static void place(lv_obj_t *obj, int x, int y, int w, int h) {
  lv_obj_set_size(obj, std::max(1, w), std::max(1, h));
  lv_obj_set_pos(obj, x, y);
}

// The colour of a rain rate in mm/h: light rain the screen's blue, heavy rain purple, a downpour red.
static theme::Role rain_role(float mm) {
  if (mm >= 10.0f) return theme::MARK_PURPLE;
  return theme::ACCENT;
}

// The tile: the icon in a circle (blue while it rains or will soon), the tile's name with the rain now beside it, a
// sentence of when it rains and for how long, and under it the forecast of the next two hours as bars.
class RainTile : public tessera::Tile {
 public:
  explicit RainTile(const tessera::Plugin *plugin) : plugin_(plugin) {}

  void create(const tessera::TileContext &c) override {
    width_ = c.width;
    height_ = c.height;
    chart_ = c.options["chart"] | true;
    step_ = std::max(1, std::min(15, c.options["step"] | 5));
    snprintf(name_, sizeof name_, "%s", c.name && *c.name ? c.name : plugin_->text("rain"));

    title_h_ = ui::line_height(Font::TITLE);
    line_h_ = ui::line_height(Font::BODY_LARGE);
    body_h_ = ui::line_height(Font::BODY);
    gap_ = ui::px(ui::large() ? 8 : 4);
    // A narrow tile (1 column): the circle on top and the sentence under it in two lines, no chart.
    compact_ = width_ < ui::text_width("00,0 mm/h", Font::BODY) * 3;
    if (compact_) {
      create_compact(c);
      return;
    }
    const int text_h = title_h_ + line_h_;
    // The chart under the words when there is room for at least a little of it.
    const int chart_room = height_ - text_h - gap_ - body_h_;
    chart_ = chart_ && chart_room >= ui::px(14);
    chart_h_ = chart_ ? std::min(chart_room, ui::mm(14)) : 0;
    const int block = text_h + (chart_ ? gap_ + chart_h_ + body_h_ : 0);
    top_ = std::max(0, (height_ - block) / 2);

    dia_ = std::min(ui::mm(13), std::max(ui::line_height(Font::ICON), text_h));
    circle_ = ui::block(c.parent, theme::TRACK);
    lv_obj_set_style_radius(circle_, LV_RADIUS_CIRCLE, 0);
    place(circle_, 0, top_ + (text_h - dia_) / 2, dia_, dia_);
    icon_ = ui::label(c.parent, Font::ICON, theme::MUTED);
    lv_obj_set_style_text_align(icon_, LV_TEXT_ALIGN_CENTER, 0);
    const int ih = ui::line_height(Font::ICON);
    place(icon_, 0, top_ + (text_h - ih) / 2, dia_, ih);

    text_x_ = dia_ + ui::px(12);
    const int text_w = width_ - text_x_;
    now_w_ = std::min(text_w / 3, ui::text_width("00,0 mm/h", Font::BODY) + ui::px(4));
    title_ = ui::label(c.parent, Font::TITLE, theme::INK);
    place(title_, text_x_, top_, text_w - now_w_ - ui::px(6), title_h_);
    now_ = ui::label(c.parent, Font::BODY, theme::MUTED);
    lv_obj_set_style_text_align(now_, LV_TEXT_ALIGN_RIGHT, 0);
    place(now_, width_ - now_w_, top_ + (title_h_ - body_h_) / 2, now_w_, body_h_);
    line_ = ui::label(c.parent, Font::BODY_LARGE, theme::INK_SOFT);
    place(line_, text_x_, top_ + title_h_, text_w, line_h_);

    if (chart_) {
      bars_ = std::min(MAX_BARS, std::max(1, 120 / step_));
      const int y = top_ + text_h + gap_;
      const int w = width_ - text_x_;
      bar_gap_ = std::max(1, ui::px(2));
      bar_w_ = std::max(1, (w - (bars_ - 1) * bar_gap_) / bars_);
      base_ = ui::block(c.parent, theme::TRACK);
      place(base_, text_x_, y + chart_h_ - ui::px(2), w, ui::px(2));
      chart_y_ = y;
      for (int i = 0; i < bars_; ++i) {
        bar_[i] = ui::block(c.parent, theme::ACCENT);
        lv_obj_set_style_radius(bar_[i], ui::px(2), 0);
      }
      static const char *const TICKS[3] = {"now", "in_1h", "in_2h"};
      for (int k = 0; k < 3; ++k) {
        tick_[k] = ui::label(c.parent, Font::BODY, theme::MUTED);
        lv_obj_set_style_text_align(tick_[k], k == 0 ? LV_TEXT_ALIGN_LEFT : k == 1 ? LV_TEXT_ALIGN_CENTER
                                                                                  : LV_TEXT_ALIGN_RIGHT, 0);
        const int tw = w / 3;
        place(tick_[k], text_x_ + (k == 0 ? 0 : k == 1 ? (w - tw) / 2 : w - tw), y + chart_h_ + ui::px(2), tw, body_h_);
        ui::set_text(tick_[k], plugin_->text(TICKS[k]));
      }
    }
  }

  void create_compact(const tessera::TileContext &c) {
    chart_ = false;
    const int text_h = 2 * body_h_;
    dia_ = std::min(ui::mm(13), std::max(ui::line_height(Font::ICON), height_ - text_h - gap_));
    dia_ = std::max(1, std::min(dia_, width_));
    top_ = std::max(0, (height_ - dia_ - gap_ - text_h) / 2);
    circle_ = ui::block(c.parent, theme::TRACK);
    lv_obj_set_style_radius(circle_, LV_RADIUS_CIRCLE, 0);
    place(circle_, (width_ - dia_) / 2, top_, dia_, dia_);
    icon_ = ui::label(c.parent, Font::ICON, theme::MUTED);
    lv_obj_set_style_text_align(icon_, LV_TEXT_ALIGN_CENTER, 0);
    const int ih = ui::line_height(Font::ICON);
    place(icon_, (width_ - dia_) / 2, top_ + (dia_ - ih) / 2, dia_, ih);
    line_ = ui::text(c.parent, Font::BODY, theme::INK_SOFT);
    lv_obj_set_style_text_align(line_, LV_TEXT_ALIGN_CENTER, 0);
    place(line_, 0, top_ + dia_ + gap_, width_, text_h);
  }

  void on_state(JsonObjectConst data) override {
    const char *state = data["state"] | "";
    have_entity_ = !data["state"].isNull();
    unavailable_ = strcmp(state, "unavailable") == 0 || strcmp(state, "unknown") == 0;
    now_rate_ = have_entity_ && !unavailable_ ? strtof(state, nullptr) : 0.0f;
    JsonObjectConst a = data["attributes"];
    start_ = a["start"] | 0u;
    end_ = a["end"] | 0u;
    minutes_ = a["minutes"] | 0.0f;
    count_ = 0;
    for (JsonVariantConst v : a["rain"].as<JsonArrayConst>()) {
      if (count_ >= MAX_BARS) break;
      rain_[count_++] = v.isNull() ? 0.0f : (v | 0.0f);
    }
  }

  void on_tick(uint32_t now) override {
    std::string line;
    bool wet = false;
    if (!have_entity_) {
      line = plugin_->text("choose");
    } else if (unavailable_) {
      line = plugin_->text("unavailable");
    } else if (now && start_ > now) {
      // Rain to come: in how long, and for how long when the sensor says.
      const long until = (start_ - now + 59) / 60;
      long length = minutes_ > 0 ? lroundf(minutes_) : end_ > start_ ? static_cast<long>((end_ - start_) / 60) : 0;
      line = length > 0 ? tessera::fill(tessera::fill(plugin_->text("soon_for"), "in", span(until)).c_str(), "for",
                                        span(length))
                        : tessera::fill(plugin_->text("soon"), "in", span(until));
      wet = until <= 60;
    } else if (now && start_ && start_ <= now) {
      // It rains now: until when, or at least the two hours the forecast reaches.
      wet = true;
      line = end_ > now ? tessera::fill(plugin_->text("until"), "in", span((end_ - now + 59) / 60))
                        : std::string(plugin_->text("long"));
    } else if (now_rate_ > 0) {
      wet = true;
      line = plugin_->text("long");
    } else {
      line = plugin_->text("dry");
    }

    ui::set_text(line_, line);
    ui::set_color(line_, theme::INK_SOFT);
    if (title_) {
      ui::set_text(title_, name_);
      ui::set_color(title_, theme::INK);
    }
    char rate[24] = "";
    if (now_rate_ > 0) {
      snprintf(rate, sizeof rate, "%.1f mm/h", now_rate_);
      if (const char *d = plugin_->text("decimal"); d && *d && *d != '.')
        for (char *p = rate; *p; ++p)
          if (*p == '.') *p = *d;
    }
    if (now_) {
      ui::set_text(now_, rate);
      ui::set_color(now_, theme::MUTED);
    }

    // The icon: pouring while it rains hard, blue in its circle while it rains or will within the hour.
    ui::set_text(icon_, ui::icon(now_rate_ >= 2.5f ? POURING : RAINY));
    ui::set_color(icon_, wet ? theme::ON_ACCENT : theme::MUTED);
    ui::set_fill(circle_, tessera::Tone::NORMAL, wet ? theme::ACCENT : theme::TRACK);

    if (chart_) paint_chart();
  }

  void on_theme() override {
    for (lv_obj_t *t : tick_)
      if (t) ui::set_color(t, theme::MUTED);
    if (base_) ui::set_fill(base_, tessera::Tone::NORMAL, theme::TRACK);
  }

 private:
  // "25 Min.", "1 Std.", "1 Std. 20 Min." in the screen's words.
  std::string span(long minutes) const {
    minutes = std::max(1L, minutes);
    if (minutes < 60) return tessera::format(plugin_->text("min"), minutes);
    const long h = minutes / 60, m = minutes % 60;
    std::string text = tessera::format(plugin_->text("hours"), h);
    if (m) text += " " + tessera::format(plugin_->text("min"), m);
    return text;
  }

  // A bar per step from now, its height by the rain rate: light rain shows, a downpour fills the chart (square root).
  void paint_chart() {
    const int room = chart_h_ - ui::px(2);
    for (int i = 0; i < bars_; ++i) {
      const float mm = i < count_ ? rain_[i] : 0.0f;
      const int h = mm > 0 ? std::max(ui::px(3), static_cast<int>(room * std::min(1.0f, sqrtf(mm / 8.0f)))) : 0;
      lv_obj_set_style_opa(bar_[i], h > 0 ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
      place(bar_[i], text_x_ + i * (bar_w_ + bar_gap_), chart_y_ + room - h, bar_w_, std::max(1, h));
      ui::set_fill(bar_[i], tessera::Tone::NORMAL, rain_role(mm));
    }
  }

  const tessera::Plugin *plugin_;
  char name_[48]{};
  int width_ = 0, height_ = 0, title_h_ = 0, line_h_ = 0, body_h_ = 0, gap_ = 0, top_ = 0, dia_ = 0;
  int text_x_ = 0, now_w_ = 0, chart_h_ = 0, chart_y_ = 0, bars_ = 0, bar_w_ = 0, bar_gap_ = 0, step_ = 5;
  bool chart_ = true, compact_ = false, have_entity_ = false, unavailable_ = false;
  uint32_t start_ = 0, end_ = 0;
  float minutes_ = 0, now_rate_ = 0;
  float rain_[MAX_BARS]{};
  int count_ = 0;
  lv_obj_t *circle_{}, *icon_{}, *title_{}, *now_{}, *line_{}, *base_{};
  lv_obj_t *bar_[MAX_BARS]{}, *tick_[3]{};
};

void DwdRain::setup() {
  add_tile("rain", [this]() { return new RainTile(this); });
}

}  // namespace esphome::dwd_rain
