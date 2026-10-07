#ifndef ADAPTIVE_PI_ARA_LOG_METADATA_PROVIDER_H_
#define ADAPTIVE_PI_ARA_LOG_METADATA_PROVIDER_H_

#include <cstdint>

namespace ara::log::detail
{

  /* Supports AP-R3-LOG-007 */
  struct RuntimeMetadata
  {
      std::int64_t unix_milliseconds;
      std::int64_t process_id;
      std::int64_t thread_id;
  };

  /* Supports AP-R3-LOG-007 */
  class MetadataProvider
  {
    public:
      virtual ~MetadataProvider() = default;

      // Return false when acquisition fails
      virtual bool Read(RuntimeMetadata& metadata) = 0;
  };

} // namespace ara::log::detail

#endif /* ADAPTIVE_PI_ARA_LOG_METADATA_PROVIDER_H_ */