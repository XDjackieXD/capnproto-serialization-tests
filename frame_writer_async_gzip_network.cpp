#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <capnp/serialize-async.h>
#include <kj/compat/gzip.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include "test.capnp.h"

class LoggingErrorHandler: public kj::TaskSet::ErrorHandler {
public:
	static LoggingErrorHandler instance;

	void taskFailed(kj::Exception&& exception) override {
		KJ_LOG(ERROR, "Uncaught exception in daemonized task.", exception);
	}
};

void acceptLoop(kj::Own<kj::ConnectionReceiver>&& listener, kj::TaskSet& tasks) {

	auto ptr = listener.get();
	tasks.add(ptr->accept().then(kj::mvCapture(kj::mv(listener),
	[&tasks](kj::Own<kj::ConnectionReceiver>&& listener, kj::Own<kj::AsyncIoStream>&& connection) {
		acceptLoop(kj::mv(listener), tasks);

		auto gzipStream = kj::heap<kj::GzipAsyncOutputStream>(*connection);
		auto message = kj::heap<capnp::MallocMessageBuilder>();
		Foo::Builder foo = message->initRoot<Foo>();

		foo.setBar(true);

		return writeMessage(*gzipStream, *message).then(kj::mvCapture(kj::mv(gzipStream),
		[](kj::Own<kj::GzipAsyncOutputStream>&& gzipStream) {
			return gzipStream->end();
		})).attach(kj::mv(connection), kj::mv(message));
	})));

}

int main (void) {
	auto ioContext = kj::setupAsyncIo();
	LoggingErrorHandler eh;
	kj::TaskSet tasks{eh};

	auto address = "localhost";
	kj::uint port = 12345;

	ioContext.provider->getNetwork().parseAddress(address, port).then(
	[&tasks](kj::Own<kj::NetworkAddress>&& addr) {
		return acceptLoop(addr->listen(), tasks);
	}).wait(ioContext.waitScope);
	kj::NEVER_DONE.wait(ioContext.waitScope);
}
