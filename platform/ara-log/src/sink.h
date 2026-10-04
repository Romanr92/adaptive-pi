#ifndef ADAPTIVE_PI_ARA_LOG_SINK_H_
#define ADAPTIVE_PI_ARA_LOG_SINK_H_

#include <string_view>

namespace ara::log::detail
{

  // Explicit status keeps backend failures internal.
  enum class SinkStatus
  {
    kSuccess,
    kWriteFailed
  };

  /* Implements AP-R3-LOG-004 */
  class Sink
  {
    public:
      virtual ~Sink() = default;

      // Pure virtual: each backend must implement submission.
      // The input borrows bytes for this call; the sink must not retain the view.
      [[nodiscard]] virtual SinkStatus Write(std::string_view record) noexcept = 0;

    protected:
      Sink() = default;

      Sink(const Sink&) = delete;
      Sink& operator=(const Sink&) = delete;
      Sink(Sink&&) = delete;
      Sink& operator=(Sink&&) = delete;
  };

} // namespace ara::log::detail

#endif /* ADAPTIVE_PI_ARA_LOG_SINK_H_ */