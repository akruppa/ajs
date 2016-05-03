ajs (asmjit superoptimiser)
===========================

This is a prototype of an assembly superoptimiser based on Petr Kobalicek's asmjit C++ library.
In order to build this using make you will need a copy of the asmjit library source (but not the tests and build scripts) in a subfolder of this directory called asmjit, for example by running:
```bash
git clone git@github.com:alexjbest/asmjit.git ~/
ln -s ~/asmjit/src/asmjit/ ~/ajs/
```
replacing `~/` with another path if required.

You can then (probably) run make to build the superoptimiser.
If this was successful `./ajs --help` will show usage information.
The helper script `superopt` is handy for optimising files which require pre-processing (via `m4` or `yasm`) its basic usage is `./superopt YOUR-FILE`, for more information run `./superopt --help`.

There is also a script called `mpir` that will automatically call `superopt` for many MPIR functions, this uses the `.txt` files whose names are given in `sigs.txt` to determine the function signatures.
