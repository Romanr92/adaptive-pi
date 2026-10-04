#ifndef ADAPTIVE_PI_ARA_LOG_CONSOLE_SINK_H_
#define ADAPTIVE_PI_ARA_LOG_CONSOLE_SINK_H_

#include "sink.h"

namespace ara::log::detail
{

  /* Implements AP-R3-LOG-004 */
  class ConsoleSink final : public Sink
  {
    public:
      ConsoleSink() noexcept = default;
      ~ConsoleSink() override = default;

      [[nodiscard]] SinkStatus Write(std::string_view record) noexcept override;
  };

} // namespace ara::log::detail

#endif /* ADAPTIVE_PI_ARA_LOG_CONSOLE_SINK_H_ */