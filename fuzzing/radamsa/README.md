# Fuzzing Souffle with Radamsa

Run `go.sh` to start a Radamsa fuzzing campaign. Inputs that cause problems for
any of the Souffle tools, along with logs, will be deposited in `./data/`.

This script currently tests:

- Evaluation
- Synthesis
- Compilation of synthesized programs
- Evaluation of compiled programs
- Profiling

It doesn't yet test:

- Profile-guided optimization

The fuzzers run in a Docker container to minimize the chance they'll interfere
with the host, and for ease of setup. There is a default memory limit of 25GB,
you can tweak this in `go.sh`. The Docker container has no network access,
because it doesn't need any. Intermediate files are written to a `tmpfs` mount,
and copied back to the host (in `./data`) if they result in a crash.
