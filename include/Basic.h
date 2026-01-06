#ifndef DASHSRV_BASIC_H__
#define DASHSRV_BASIC_H__

#include <stdint.h>
#include <stddef.h>
#include <functional>

#define DASHSRV_VERSION "1.0"

#define UNUSED __attribute__((unused))

#define BIND_THIS(fn) std::bind(&fn, this, std::placeholders::_1)
#define BIND_THIS2(fn) std::bind(&fn, this, std::placeholders::_1, std::placeholders::_2)

uint64_t GetTimeMillis();

#endif // DASHSRV_BASIC_H__
