#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <capnp/serialize-async.h>
#include <kj/io.h>
#include <kj/async-io.h>
#include <kj/async-unix.h>
#include <kj/compat/gzip.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include "test.capnp.h"

kj::Promise<bool> reader(kj::AsyncInputStream& asyncStdin, kj::AsyncIoContext& ioContext) {
	return capnp::tryReadMessage(asyncStdin).then(
	[&](kj::Maybe<kj::Own<capnp::MessageReader>>&& message) -> bool {
		KJ_IF_MAYBE(m, message) {	
			Foo::Reader foo = kj::mv(*m)->getRoot<Foo>();
			printf("Foo: bar=%d\n", foo.getBar());
			return true;
		} else {
			return false;
		}
	});
}

int main (void) {
	auto ioContext = kj::setupAsyncIo();
	
	kj::Own<kj::AsyncInputStream> asyncStdin = ioContext.lowLevelProvider->wrapInputFd(STDIN_FILENO);
	kj::GzipAsyncInputStream gzipStdin{*asyncStdin};

	auto promise = reader(gzipStdin, ioContext);
	while (true) {
		if (promise.poll(ioContext.waitScope)) {
			if (promise.wait(ioContext.waitScope)) {
				promise = reader(gzipStdin, ioContext);
			} else {
				break;
			}
		}
	}
}
