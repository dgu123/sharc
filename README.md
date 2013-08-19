SHARC
======

Simple Highspeed Archiver<br/>www.centaurean.com/sharc

SHARC is a very fast file archiver with one goal in mind : speed.
On an Intel Core i7 it reaches compression speeds of <b>600 MB/s</b> and decompression speeds of <b>1 GB/s</b>, and that is <b>PER core</b> !
Compression ratio is typically at around 50-60 % with the fastest algorithm.

Usage
------

To compress a file named 'test' :
> sharc test

To decompress 'test.sharc'
> sharc -d test.sharc

Options
--------

It is possible to choose the compression algorithm. The fastest (default) can be specified like this :
> sharc -c0 test

There also is a slightly slower algorithm (20 % slower), with a better compression ratio (5 to 25 % better) :
> sharc -c1 test


For more information please refer to www.centaurean.com/sharc
