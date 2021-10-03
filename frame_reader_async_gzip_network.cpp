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
	auto address = "localhost";
	kj::uint port = 12345;

	kj::Own<kj::AsyncIoStream> networkStream = ioContext.provider->getNetwork().parseAddress(address, port).then(
	[](kj::Own<kj::NetworkAddress>&& addr) {
		return addr->connect().attach(kj::mv(addr));
	}).wait(ioContext.waitScope);

	kj::GzipAsyncInputStream gzipStream{*networkStream};

	auto promise = reader(gzipStream, ioContext);
	while (true) {
		if (promise.poll(ioContext.waitScope)) {
			if (promise.wait(ioContext.waitScope)) {
				promise = reader(gzipStream, ioContext);
			} else {
				break;
			}
		}
	}
}
