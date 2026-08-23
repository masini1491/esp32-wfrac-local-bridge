#include "wfrac_time_espidf/esp_system_time_provider.hpp"

#include <cstdint>
#include <limits>
#include <sys/time.h>

namespace wfrac_time_espidf {
namespace {

static_assert(sizeof(decltype(timeval{}.tv_sec)) >= sizeof(int64_t),
              "WF-RAC timestamps require a 64-bit-capable POSIX system clock");

bool is_usable_quality(wfrac_api::TimeQuality quality) {
  return quality == wfrac_api::TimeQuality::estimated || quality == wfrac_api::TimeQuality::trusted;
}

bool is_positive_timeval(const timeval& value) {
  return value.tv_sec > 0;
}

bool timeval_to_unix_seconds(const timeval& value, int64_t* unix_seconds) {
  if (unix_seconds == nullptr || !is_positive_timeval(value)) return false;
  const uintmax_t seconds = static_cast<uintmax_t>(value.tv_sec);
  if (seconds > static_cast<uintmax_t>(std::numeric_limits<int64_t>::max())) return false;
  *unix_seconds = static_cast<int64_t>(seconds);
  return true;
}

bool can_store_unix_seconds(int64_t unix_seconds) {
  if (unix_seconds <= 0) return false;
  return static_cast<uintmax_t>(unix_seconds) <=
         static_cast<uintmax_t>(std::numeric_limits<decltype(timeval{}.tv_sec)>::max());
}

}  // namespace

wfrac_api::TimeSample EspSystemTimeProvider::now() {
  if (quality_ == wfrac_api::TimeQuality::unavailable) return {};
  timeval value{};
  int64_t unix_seconds = 0;
  if (gettimeofday(&value, nullptr) != 0 || !timeval_to_unix_seconds(value, &unix_seconds)) return {};
  return {unix_seconds, quality_};
}

EspSystemTimeError EspSystemTimeProvider::seed(wfrac_api::TimeSample sample) {
  if (!is_usable_quality(sample.quality) || !can_store_unix_seconds(sample.unix_seconds)) {
    return EspSystemTimeError::invalid_argument;
  }
  timeval value{};
  value.tv_sec = static_cast<decltype(value.tv_sec)>(sample.unix_seconds);
  value.tv_usec = 0;
  if (settimeofday(&value, nullptr) != 0) return EspSystemTimeError::set_failed;
  quality_ = sample.quality;
  return EspSystemTimeError::none;
}

EspSystemTimeError EspSystemTimeProvider::adopt_current_system_time(wfrac_api::TimeQuality quality) {
  if (!is_usable_quality(quality)) return EspSystemTimeError::invalid_argument;
  timeval value{};
  if (gettimeofday(&value, nullptr) != 0 || !is_positive_timeval(value)) {
    return EspSystemTimeError::system_time_unavailable;
  }
  quality_ = quality;
  return EspSystemTimeError::none;
}

void EspSystemTimeProvider::invalidate() {
  quality_ = wfrac_api::TimeQuality::unavailable;
}

}  // namespace wfrac_time_espidf
