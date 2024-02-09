# Fuzzing Souffle with AFL

Run `go.sh` to start an AFL fuzzing campaign. Files produced by AFL will be
deposited in `./data/`. You may optionally run `go-min.sh` first to generate a
smaller initial corpus of test cases.

This harness only tests parsing, typechecking, and evaluation with the main
`souffle` binary.

AFL is run in a Docker container to minimize the chance interfere with the host,
and for ease of setup. There is a default memory limit of 25GB, you can tweak
this in `go.sh`. The Docker container has no network access, because it doesn't
need any. Intermediate files are written to a `tmpfs` mount.
