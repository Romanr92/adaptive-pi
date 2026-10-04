#include "adaptive_pi/platform_test/build_info.hpp"

#include <cassert>
#include <cstddef>

extern std::size_t nondet_size();

/* Application-local metadata invariant; no AUTOSAR requirement ID.
 * For every valid index, the returned views contain the expected character.
 * The index is unconstrained; the branch restricts access to each view's bounds.
 * This checks the real build_info.cpp implementation, not console I/O or main.
 */
int main()
{
  /* Arrange */
  constexpr char expected_name[] = "platform-test-service";
  constexpr char expected_version[] = "0.2.0";
  const auto index = nondet_size();

  /* Act */
  const auto name = adaptive_pi::platform_test::ApplicationName();
  const auto version = adaptive_pi::platform_test::Version();

  /* Assert */
  assert(name.size() == sizeof(expected_name) - 1);
  assert(version.size() == sizeof(expected_version) - 1);

  if (index < name.size())
  {
    assert(name[index] == expected_name[index]);
  }
  if (index < version.size())
  {
    assert(version[index] == expected_version[index]);
  }
}
