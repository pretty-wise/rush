/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once
#include "base/core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct context *punchserver_t;

punchserver_t punch_server_create(u32 addr, u16 *port);
void punch_server_destroy(punchserver_t ctx);
void punch_server_update(punchserver_t ctx, u32 dt);

#ifdef __cplusplus
} // extern "C"
#endif
