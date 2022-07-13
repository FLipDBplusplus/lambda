#include <config/board.h>

namespace Ion {
namespace Device {
namespace Board {

/* N0100 userland DFU can't write:
 * - the SRAM allocated for the kernel
 * - the Userland stack where the DFU driver is actually living (it cannot
 *   override itself) */

uint32_t writableSRAMStartAddress() {
  return Config::UserlandSRAMOrigin;
}

uint32_t writableSRAMEndAddress() {
  return Config::UserlandSRAMOrigin + Config::UserlandSRAMLength - Config::UserlandStackLength;
}

}
}
}

