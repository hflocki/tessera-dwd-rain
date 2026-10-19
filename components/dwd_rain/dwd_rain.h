#pragma once
#include "esphome/core/component.h"
#include "esphome/components/smart_display/plugin_api.h"

namespace esphome::dwd_rain {

// The plugin: one tile type, "rain" (plugin:dwd_rain.rain in a layout).
class DwdRain : public Component, public tessera::Plugin {
 public:
  void setup() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
};

}  // namespace esphome::dwd_rain
