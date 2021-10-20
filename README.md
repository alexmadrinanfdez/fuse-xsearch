# fuse-xsearch

Integration of XSearch with a local FUSE file system.

The goal is to integrate XSearch within a file system to explore what kind of impact in performance it produces.

## Related work

_XSearch_ is a scalable solution for achieving infor-mation retrieval in large-scale storage systems.
It focuses on indexing speed since it is focused on an academia audience that may prioritize availability over initial delay in their searches.
The project's library is called [Ouroboros](https://gitlab.com/xsearch/ouroboroslib).

_FUSE_ (Filesystem in USErspace) is a protocol between the kernel and a user-space process,
letting the user-space serve file system requests coming from the kernel.
They are the standard interface for Unix-like computer operating systems.
It allows non-privileged users to manage their own file system.
[libfuse](https://github.com/libfuse/libfuse) can be used to implement such file systems.
It provides the means to communicate with the FUSE kernel module.
