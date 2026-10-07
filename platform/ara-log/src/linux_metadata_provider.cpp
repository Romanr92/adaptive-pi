#include "linux_metadata_provider.h"

#include <cstdint>
#include <limits>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

namespace ara::log::detail
{

  /* Implements production metadata acquisition for AP-R3-LOG-007 */
  bool LinuxMetadataProvider::Read(RuntimeMetadata& metadata) noexcept
  {
    constexpr std::int64_t c_miliseconds_per_second = 1000;
    constexpr std::int64_t c_nanoseconds_per_millisecond = 1000000;

    bool success = false;
    timespec timestamp{};

    // read wall-clock time
    if (::clock_gettime(CLOCK_REALTIME, &timestamp) == 0)
    {
      const auto seconds = static_cast<std::int64_t>(timestamp.tv_sec);
      const auto milliseconds = static_cast<std::int64_t>(timestamp.tv_nsec) / c_nanoseconds_per_millisecond;

      const auto process_id = ::getpid();
      const auto thread_id = ::syscall(SYS_gettid);

      // Check coversion bounds before multiplying.
      const auto maximum = std::numeric_limits<std::int64_t>::max();
      const auto minimum = std::numeric_limits<std::int64_t>::min();

      if ((process_id > 0) && (thread_id > 0) && (seconds >= (minimum / c_miliseconds_per_second)) &&
          (seconds <= ((maximum - milliseconds) / c_miliseconds_per_second)))
      {
        // Publish only complete metadata
        metadata = RuntimeMetadata{seconds * c_miliseconds_per_second + milliseconds,
                                   static_cast<std::int64_t>(process_id), static_cast<std::int64_t>(thread_id)};
        success = true;
      }
    }

    return success;
  }
} // namespace ara::log::detail
