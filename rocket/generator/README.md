This directory contains a Makefile and the necessary sources to generate the
Rocket cores used for benchmarking. See the `Makefile` for more details. The
cores can be regenerated as follows:

```
make all
```

You may want to delete the `rocket-chip-*` directory to cause the Makefile to
re-clone an up-to-date version of the upstraem repository.
