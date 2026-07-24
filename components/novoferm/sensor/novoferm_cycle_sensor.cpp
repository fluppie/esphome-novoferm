#include "novoferm_cycle_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace novoferm {

static constexpr const char *const TAG = "novoferm.sensor";

void NovofermCycleSensor::setup() {
  this->parent_->set_cycle_count_listener(
      [this](uint16_t count) { this->publish_state(count); });
}

void NovofermCycleSensor::update() { this->parent_->request_cycle_count(); }

void NovofermCycleSensor::dump_config() {
  LOG_SENSOR("", "Novoferm Cycle Sensor", this);
  LOG_UPDATE_INTERVAL(this);
}

}  // namespace novoferm
}  // namespace esphome
