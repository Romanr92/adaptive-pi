#ifndef ADAPTIVE_PI_ARA_LOG_LINUX_METADATA_PROVIDER_H_
#define ADAPTIVE_PI_ARA_LOG_LINUX_METADATA_PROVIDER_H_

#include "metadata_provider.h"

namespace ara::log::detail
{

  /* Supports AP-R3-LOG-007 */
  class LinuxMetadataProvider final : public MetadataProvider
  {
    public:
      // Read the current timestamp and Linux IDs.
      bool Read(RuntimeMetadata& metadata) noexcept override;
  };

} // namespace ara::log::detail

#endif /* ADAPTIVE_PI_ARA_LOG_LINUX_METADATA_PROVIDER_H_ */