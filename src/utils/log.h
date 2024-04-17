#pragma once

#include <assert.h>

#define inu_assert_msg(msg) assert(false, msg)
#define inu_assert(exp, msg) assert(exp, msg)
