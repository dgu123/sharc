SHARC <a href="http://www.centaurean.com"><img src="http://www.centaurean.com/images/centaurean.png" width="140" align="right" title="Centaurean"/></a>
======

Simple Highspeed Archiver<br/>www.centaurean.com/sharc

SHARC is a very fast file archiver with one goal in mind : speed.
On an Intel Core i7-2600K @ 3.40 GHz it reaches compression speeds of <b>700 MB/s</b> and decompression speeds of <b>1 GB/s</b>, and that is <b>PER core</b> !
Compression ratio is typically at around 50-60 % with the fastest algorithm.

A single-thread <b>command line interface</b> is shipped by default, as well as <b>two APIs</b> for  use of SHARC as a library.

Command line interface
-----------------------

To compress a file named 'test' :
> sharc test

To decompress 'test.sharc'
> sharc -d test.sharc

It is of course possible to choose the compression algorithm. The fastest (default) can be specified like this :
> sharc -c0 test

There is also a slightly slower algorithm (20 % slower), with a better compression ratio (5 to 25 % better) :
> sharc -c1 test

For more help and the full list of options :
> sharc -h

Applications programming interfaces
------------------------------------

SHARC is equipped with a <b>stream API</b> very similar to bzip's and a <b>buffers API</b>. Please see <a href="https://github.com/centaurean/sharc/blob/master/src/api.h">api.h</a> for specifications and examples.

The <b>stream API</b> provides fine-grained control on the encoding/decoding process and enables extremely versatile uses (very fast large files compression, on-the-fly compression for networks, etc.).

The <b>buffers API</b> is a simpler buffer to buffer compression/decompression tool and will fit more typical uses.

More information
-----------------
For more information please refer to <a href="www.centaurean.com/sharc">www.centaurean.com/sharc</a>
