#include "ara/core/error_code.h"

#include <cassert>
#include <cstdint>

extern std::int32_t nondet_int32();
extern std::uint64_t nondet_uint64();

#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
namespace
{
  void UnusedConverter(const ara::core::ErrorCode&)
  {
    std::terminate();
  }
} // namespace
#endif

/* Supplementary proof for AP-R3-CORE-003 and AP-R3-CORE-004.
 * All int32 error values and uint64 domain identifiers are symbolic.
 * Domains have fixed non-empty names and outlive their error codes.
 * Verify exact value/domain retention and equality by value plus domain ID.
 * Exception conversion and invalid domain construction are outside this proof.
 */
int main()
{
  /* Arrange */
  const auto first_id = nondet_uint64();
  const auto second_id = nondet_uint64();
  const auto first_value = nondet_int32();
  const auto second_value = nondet_int32();
#if ADAPTIVE_PI_EXCEPTIONS_ENABLED
  const ara::core::ErrorDomain first_domain{first_id, "first", UnusedConverter};
  const ara::core::ErrorDomain second_domain{second_id, "second", UnusedConverter};
#else
  const ara::core::ErrorDomain first_domain{first_id, "first"};
  const ara::core::ErrorDomain second_domain{second_id, "second"};
#endif
  /* Act */
  const ara::core::ErrorCode first{first_value, first_domain};
  const ara::core::ErrorCode second{second_value, second_domain};

  /* Assert */
  assert(first.Value() == first_value);
  assert(second.Value() == second_value);
  /* Bind returned references explicitly for ESBMC 8.5 reference lowering. */
  const auto& first_origin = first.Domain();
  const auto& second_origin = second.Domain();
  assert(&first_origin == &first_domain);
  assert(&second_origin == &second_domain);
  const bool expected_equal = first_value == second_value && first_id == second_id;
  assert((first == second) == expected_equal);
  assert((first != second) == !expected_equal);
}
