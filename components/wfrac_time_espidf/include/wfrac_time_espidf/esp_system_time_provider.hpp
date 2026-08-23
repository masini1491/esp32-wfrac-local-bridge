#pragma once

#include "wfrac_api/time_provider.hpp"

namespace wfrac_time_espidf {

enum class EspSystemTimeError {
  none,
  invalid_argument,
  system_time_unavailable,
  set_failed,
};

class EspSystemTimeProvider final : public wfrac_api::ITimeProvider {
 public:
  EspSystemTimeProvider() = default;

  wfrac_api::TimeSample now() override;

  // Sets process/device-wide ESP-IDF POSIX system time on success.
  EspSystemTimeError seed(wfrac_api::TimeSample sample);

  // Adopts an already-set POSIX system clock without changing it.
  EspSystemTimeError adopt_current_system_time(wfrac_api::TimeQuality quality);

  // Only clears this provider's RAM-held quality; it does not change system time.
  void invalidate();

 private:
  wfrac_api::TimeQuality quality_ = wfrac_api::TimeQuality::unavailable;
};

}  // namespace wfrac_time_espidf
