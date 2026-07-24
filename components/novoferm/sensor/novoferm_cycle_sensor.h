#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "../novoferm.h"

namespace esphome {
namespace novoferm {

// Reports the unit's cycle counter (completed openings), read from the
// byte-registers CYCLES_HIGH (0x0E) and CYCLES_LOW (0x0F).
//
// The value refreshes automatically shortly after every finished gate
// movement (handled by the Novoferm hub); update_interval (default
// 60min) acts as a periodic safety net.
class NovofermCycleSensor : public sensor::Sensor, public PollingComponent {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;

  void set_novoferm_parent(Novoferm *parent) { this->parent_ = parent; }

 protected:
  Novoferm *parent_{nullptr};
};

}  // namespace novoferm
}  // namespace esphome
