#ifndef ARA_LOG_TRACE_ARTIFACT_H_
#define ARA_LOG_TRACE_ARTIFACT_H_

namespace ara::log::ext
{
  /* Supports AP-R3-LOG-011 */
  template <typename MsgId, typename... Params>
  /* Implemented DD-12 */
  void TraceArti(const MsgId& message, const Params&... params) noexcept;
} // namespace ara::log::ext

#endif /* ARA_LOG_TRACE_ARTIFACT_H_ */