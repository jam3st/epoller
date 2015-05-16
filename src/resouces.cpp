#include <unistd.h>
#include "resouces.hpp"

namespace Sb {
      size_t const Resouces::pageSize = ::sysconf(_SC_PAGE_SIZE);
}
