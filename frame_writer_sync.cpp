#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <capnp/serialize.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include "test.capnp.h"

int main (void) {
	::capnp::MallocMessageBuilder message;
	Foo::Builder foo = message.initRoot<Foo>();

	foo.setBar(true);

	writeMessageToFd(STDOUT_FILENO, message);

	foo.setBar(false);

	writeMessageToFd(STDOUT_FILENO, message);
}
