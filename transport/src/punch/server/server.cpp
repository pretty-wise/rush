/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "rush/punch/server.h"
#include "base/network/url.h"
#include "punch_server.h"

struct context {
	Rush::Punch::PunchServer actual;
};

punchserver_t punch_server_create(u32 addr, u16* port) {
	context* ctx = new context;

	Base::Url url(addr, *port);

	if(!ctx->actual.Open(url, port)) {
		delete ctx;
		return nullptr;
	}
	return ctx;
}

void punch_server_destroy(punchserver_t ctx) {
	delete ctx;
}

void punch_server_update(punchserver_t ctx, u32 dt) {
	if(!ctx) {
		return;
	}
	ctx->actual.Update(dt);
}
