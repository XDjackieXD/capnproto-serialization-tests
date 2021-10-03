#include <capnp/message.h>
#include <capnp/serialize.h>
#include <kj/io.h>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include "test.capnp.h"

int main (void) {
	while (true) {
		try {
			capnp::StreamFdMessageReader messageReader{STDIN_FILENO};
			Foo::Reader foo = messageReader.getRoot<Foo>();
			printf("Foo: bar=%d\n", foo.getBar());
		} catch (std::exception e) {
			fprintf(stderr, "Kj Exception: %s\n", e.what());
			break;
		}
	}
}
