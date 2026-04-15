#ifndef BAD_RTOS_PLATFORM_INLCUDE_H
#define BAD_RTOS_PLATFORM_INCLUDE_H

#ifdef BAD_PLATFORM_F411
#include "badrtos_armv7.h"
#include "badhal_f411.h"
#endif

#ifdef BAD_PLATFORM_H562
#include "badrtos_armv8.h"
#include "badhal_h562.h"
#endif

#ifdef BAD_PLATFORM_H562T
#include "badrtos_armv8_trustzone.h"
#include "badhal_h562.h"
#endif

#endif

