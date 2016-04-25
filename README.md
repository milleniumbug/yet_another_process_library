# yet_another_process_library

A library for launching processes, very heavily based on [tiny-process-library](https://github.com/eidheim/tiny-process-library). It shares much of the code with it, to the point it could be yet another fork of it.

However, it's not, due to having incompatible interface. Notably:

- Boost.Filesystem is a required dependency.
- it launches processes directly, without using shell as a mediator.