/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "../include/client.h"
#include "punch_client.h"

#include "base/core/macro.h"

static const Base::LogChannel kPunchLog("punch_client");

struct context {
  template <typename T> context(T &&arg) : actual(arg) {}

  Rush::Punch::PunchClient actual;
};

#ifdef __cplusplus
extern "C" {
#endif

punch_t punch_create(const PunchConfig &config) {
  if(!Rush::Punch::IsValid(config)) {
    return nullptr;
  }
  return new context(config);
}

void punch_destroy(punch_t context) { delete context; }

void punch_update(punch_t context, u32 delta_ms) {
  if(!context) {
    return;
  }
  context->actual.Update(delta_ms);
}
void punch_read(punch_t context, s8 *buffer, streamsize nbytes,
                const Base::Socket::Address &from) {
  if(!context) {
    return;
  }
  context->actual.Read(buffer, nbytes, from);
}

int punch_resolve_public_address(punch_t context, u32 timeout_ms) {
  return context->actual.ResolvePublicAddress(timeout_ms);
}

int punch_connect(punch_t context, const Base::Url &a_private,
                  const Base::Url &a_public, const Base::Url &b_private,
                  const Base::Url &b_public) {
  return context->actual.Connect(a_private, a_public, b_private, b_public);
}

void punch_abort(punch_t ctx, punch_op id) { ctx->actual.Abort(id); }

#ifdef __cplusplus
} // extern "C"
#endif
