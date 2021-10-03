#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <capnp/serialize.h>
#include <kj/compat/gzip.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include "test.capnp.h"

int main (void) {
	kj::FdOutputStream outStream{STDOUT_FILENO};
	kj::GzipOutputStream gzipOutStream{outStream};

	capnp::MallocMessageBuilder message;
	Foo::Builder foo = message.initRoot<Foo>();

	foo.setBar(true);

	//writePackedMessageToFd(STDOUT_FILENO, message);
	writeMessage(gzipOutStream, message);

	foo.setBar(false);

	//writePackedMessageToFd(STDOUT_FILENO, message);
	writeMessage(gzipOutStream, message);
}
