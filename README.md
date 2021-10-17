# fuse-xsearch

Integration of XSearch with a local FUSE file system.

The goal is to integrate XSearch within a file system to explore what kind of impact in performance it produces.

## Background information

_XSearch_ is “a scalable solution for achieving infor-mation retrieval in large-scale storage systems”.
It focuses on indexing speed since it is focused on an academia audience that may prioritize availability over initial delay in their searches.

_libfuse_ is a library to implement file systems.
It is based on the FUSE protocol, “a protocol between the kernel and a user-space process, 
letting theuser-space serve file system requests coming from the kernel”. 
They are the standard interface for Unix-like computer operating systems. 
It allows non-privileged users to manage their own file system.
