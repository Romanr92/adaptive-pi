#ifndef ARA_CORE_CORE_FWD_H_
#define ARA_CORE_CORE_FWD_H_

namespace ara::core
{

  class ErrorDomain;
  class ErrorCode;

  template <typename T, typename E = ErrorCode> class Result;

} // namespace ara::core

#endif // ARA_CORE_CORE_FWD_H_