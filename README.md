# Sandboxed app example

This is an example app for education purposes. It shows how to run a containarized sandboxed application using Linux primitives such as namespaces, cgroups, capabilities and seccomp.

There is also an accompanying blog post "[Implementing a Docker like sandbox in Linux, from scratch](https://ankursharma.vercel.app/blogs/linux_sandbox_from_scratch)" which introduces the context behind this app.

## Usage:

```bash
nix-shell shell.nix
./configure.sh
./build_and_run.sh
```

