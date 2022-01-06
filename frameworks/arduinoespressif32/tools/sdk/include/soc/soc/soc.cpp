#include "soc.h"

#include "glog/logging.h"

extern "C" {

uint32_t __readPeriReg(uint32_t reg) {
  LOG(FATAL) << "Reading directly from registers is not currently supported.";
}

void __writePeriReg(uint32_t reg, uint32_t val) {
  LOG(FATAL) << "Writing directly to registers is not currently supported.";
}

}
