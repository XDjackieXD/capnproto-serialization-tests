#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <capnp/serialize-async.h>
#include <kj/compat/gzip.h>
#include <kj/compat/tls.h>
#include <kj/filesystem.h>
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

struct Data {
	kj::Own<kj::AsyncIoStream> connection;
	kj::Own<kj::GzipAsyncOutputStream> gzipStream;
	kj::Own<capnp::MessageBuilder> message;
};

void acceptLoop(kj::Own<kj::ConnectionReceiver>&& listener, kj::TaskSet& tasks) {

	auto ptr = listener.get();
	tasks.add(ptr->accept().then(kj::mvCapture(kj::mv(listener),
	[&tasks](kj::Own<kj::ConnectionReceiver> listener, kj::Own<kj::AsyncIoStream> connection) {
		acceptLoop(kj::mv(listener), tasks);

		auto dataStruct = kj::heap<Data>();

		dataStruct->connection = kj::mv(connection);
		dataStruct->gzipStream = kj::heap<kj::GzipAsyncOutputStream>(*dataStruct->connection);
		dataStruct->message = kj::heap<capnp::MallocMessageBuilder>();

		Foo::Builder foo = dataStruct->message->initRoot<Foo>();
		foo.setBar(true);

		return writeMessage(*dataStruct->gzipStream, *dataStruct->message).then(kj::mvCapture(kj::mv(dataStruct),
		[](kj::Own<Data> dataStruct) {
			return dataStruct->gzipStream->end().then(kj::mvCapture(kj::mv(dataStruct),
			[](kj::Own<Data> dataStruct) {
				dataStruct->connection->shutdownWrite();
			})).eagerlyEvaluate(nullptr);
		}));
	})));

}

int main (void) {
	auto ioContext = kj::setupAsyncIo();
	LoggingErrorHandler eh;
	kj::TaskSet tasks{eh};

	auto address = "localhost";
	kj::uint port = 12345;

	auto fs = kj::newDiskFilesystem();
	kj::Path certPath{"cert.pem"};
	auto certFile = fs->getCurrent().openFile(certPath);
	kj::Path keyPath{"key.pem"};
	auto keyFile = fs->getCurrent().openFile(keyPath);

	kj::TlsContext::Options tlsOptions{};
	kj::TlsCertificate tlsCert{certFile->readAllText()};
	kj::TlsPrivateKey tlsKey{keyFile->readAllText()};
	kj::TlsKeypair tlsKeypair{tlsKey, tlsCert};

	tlsOptions.trustedCertificates = {tlsCert};
	tlsOptions.defaultKeypair = tlsKeypair;
	kj::TlsContext tlsContext{tlsOptions};

	ioContext.provider->getNetwork().parseAddress(address, port).then(
	[&tasks, &tlsContext](kj::Own<kj::NetworkAddress>&& addr) {
		return acceptLoop(tlsContext.wrapPort(addr->listen()), tasks);
	}).wait(ioContext.waitScope);
	kj::NEVER_DONE.wait(ioContext.waitScope);
}
