# chessh alternate frontends

The main frontend to chessh is ssh. After all, it's right there in the name. The
goal of chessh, however, is to create the most portable online chess game ever
created. If you're using a PDP-7 from 1965 with 18 bit bytes that's somehow been
connected to the internet, you deserve to play chess just like everybody else,
even if your computer is too slow to handle modern encryption protocols.

That's why I'm also supporting telnet, and maybe
[Gopher](https://en.wikipedia.org/wiki/Gopher_(protocol)) eventually.

These frontends are meant to be short little hacks that get the job done in
Docker and nothing more. Hard-coded directory paths are fine, don't worry about
it.
