#ifndef ARA_LOG_TRACE_ROUTING_H_
#define ARA_LOG_TRACE_ROUTING_H_

namespace ara::log
{
  /* Supports AP-R3-LOG-011 */
  template <typename MsgId>
  struct TraceRouting
  {
      static constexpr bool kLoggerEnabled = true;
      static constexpr bool kTraceEnabled = false;
  };

} // namespace ara::log

#endif /* ARA_LOG_TRACE_ROUTING_H_ */