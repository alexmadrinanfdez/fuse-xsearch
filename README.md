# fuse-xsearch

Integration of XSearch with a local FUSE file system.

The goal is to integrate XSearch within a file system to explore what kind of impact in performance it produces.

_XSearch_ is a scalable solution for achieving information retrieval in large-scale storage systems.
It focuses on indexing speed since it is focused on an academia audience that may prioritize availability over initial delay in their searches.
The project's library is called [Ouroboros](https://gitlab.com/xsearch/ouroboroslib).

_FUSE_ (Filesystem in USErspace) is a protocol between the kernel and a user-space process,
letting the user-space serve file system requests coming from the kernel.
They are the standard interface for Unix-like computer operating systems.
It allows non-privileged users to manage their own file system.
[libfuse](https://github.com/libfuse/libfuse) can be used to implement such file systems.
It provides the means to communicate with the FUSE kernel module.

## Set-up

The _passthrough_ file system mirrors the contents of the root directory (`/`) under the mountpoint. Conversely, any changes made under the mountpoint will be reflected in the root directory.

To mount, execute in the terminal;

```bash
gcc -Wall passthrough.c -o passthrough `pkg-config fuse3 --cflags --libs`
./passthrough -f <mountpoint>
```

The `-f` option will print debug messages specified via `printf`.

If the feedback option is not used, to unmount;

```bash
fusermount -u <mountpoint>
```

### Virtual Machine

The project targets Linux kernels. To run it in a virtual Linux environment, one possibility is to use virtual machines.
The repository includes a `Dockerfile` to execute the program in a Docker virtual machine.
To run the container, make sure you have `Docker` installed and execute in the terminal:

```bash
docker build -t <tag> .
docker run -it --cap-add SYS_ADMIN --device /dev/fuse <tag>
```

where `<tag>` is simply a custom name given to the image.

#### Test

The more troubling dependency is `libfuse` because it needs to comunicate directly with the FUSE kernel module. 
To ensure a proper setup, run in the CLI of the container:

```bash
cd fuse-3.10.5/build
python3 -m pytest test/
```

...and make sure it does not skip (all) the tests. Take into account the `test_cuse` test will fail because the `cuse` device is not present in the system.

After that, a proper filesystem can be mounted inside the `example` directory by executing any example file system.

```bash
cd example
mkdir -p <mountpoint>
./<example> <mountpoint>
cd <mountpoint>
```