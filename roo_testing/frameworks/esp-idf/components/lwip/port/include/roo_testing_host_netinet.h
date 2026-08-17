#pragma once

// This file deliberately lives in ESP-IDF's final public lwIP include root.
// include_next therefore skips IDF's netinet compatibility headers and reaches
// the C library headers used by the Linux socket ABI.
#include_next <netinet/in.h>
#include_next <netinet/tcp.h>

// The libc arpa/netdb headers include netinet/in.h themselves. Their lookup
// starts from the full include path and would otherwise re-enter ESP-IDF's
// forwarding wrappers before reaching libc. Mark those wrappers consumed once
// the real netinet ABI above is available.
#ifndef IN_H_
#define IN_H_
#endif
#ifndef INET_H_
#define INET_H_
#endif

#include_next <arpa/inet.h>
#include_next <netdb.h>
