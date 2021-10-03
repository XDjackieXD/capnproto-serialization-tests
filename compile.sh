#!/bin/bash

mkdir -p build

files=( frame_reader_sync frame_reader_async frame_reader_async_gzip frame_reader_async_gzip_network frame_reader_async_gzip_tls_network frame_writer_sync frame_writer_sync_gzip frame_writer_async_gzip_network frame_writer_async_gzip_tls_network )

for file in "${files[@]}"
do
	g++ "$file.cpp" -lkj -lcapnp -lkj-async -lcapnp-rpc -lkj-gzip -lkj-tls -o "build/$file"
done
