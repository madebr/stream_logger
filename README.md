# stream_logger

This program will log stdin/stdout/stderr of a process.
This might be useful for debugging purposes.
`stream_logger` tries to propagate all stdin/stderr/stdout, as is the exit code.

## Usage

```sh
$ ./stream_logger --help
Run and log process.

stream_logger [--name_prefix PREFIX] -- PROGRAM [ARG ...]

Generic options:
  --help                   Output this help message
  --name_prefix arg (=log) Output name prefix

```

## Example

```sh
$ cat << EOF | ./stream_logger -- m4
define(a,4)dnl
a
EOF
```
will print:
```
4
```
to the stdout.

Furthermore, `log_args_000`, `log_stdin_000`, `log_stdout_000` and `log_stderr_000` will be created.
 
## Pitfalls
Some programs will not finish immediately when calling its exit function.
e.g. Python will continue parsing stdin until EOF.

```sh
$ cat << EOF | ./stream_logger -- python
print(4)
import sys
sys.exit(3)
# Code after sys.exit will be parsed as well, but not executed
print(40)
EOF
```

## Build requirements
- C++14 compiler
- Boost (asio, process, program_options)
