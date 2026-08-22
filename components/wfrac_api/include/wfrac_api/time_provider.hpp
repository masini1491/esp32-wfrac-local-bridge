#pragma once

#include <cstdint>

namespace wfrac_api {

enum class TimeQuality { unavailable, estimated, trusted };

struct TimeSample {
  int64_t unix_seconds = 0;
  TimeQuality quality = TimeQuality::unavailable;
};

class ITimeProvider {
 public:
  virtual ~ITimeProvider() = default;
  virtual TimeSample now() = 0;
};

}  // namespace wfrac_api
