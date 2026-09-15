#include "adaptive_pi/platform_test/build_info.hpp"

#include <iostream>

int main()
{
  std::cout << adaptive_pi::platform_test::ApplicationName() << " v" << adaptive_pi::platform_test::Version() << '\n';

  return 0;
}