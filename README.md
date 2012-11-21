eins - A tool for measuring network bandwidths and latencies
============================================================

This is a handy tool I wrote back in 2006 as part of my diploma thesis at the University of Potsdam for measuring bandwidths and latencies on various types of networks. It’s intended to be easily extendable with new types.

Currently, following types are supported:

- NEON (internal project at UP)
- SCTP
- TCP
- TCP, just connecting
- UDP
- malloc() + memcpy() + free()

The name is a backronym for “eins is not sockping” which was the predecessor by [Lars
Schneidenbach](http://www.larsschneidenbach.de) that it is based on.

> As of writing this, it has been *six years* since I did anything with it thus I have no idea if it still works at all. As a true programmer prima donna, I don’t dare to look inside since it might hurt my vain feelings. Therefore, use it on your own discretion.  I’m just putting it on GitHub so it doesn’t get lost, pull requests are welcome though.

For cli arguments, just run `eins` without any of them.

It’s licensed as GPLv3.  Sorry, I didn’t knew better back then. :(
