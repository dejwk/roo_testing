#include <sys/reent.h>

extern "C" {

struct _reent * __getreent (void) {
  static _reent reent;
  return &reent;
}

}