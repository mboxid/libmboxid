
![logo](./doc/mboxid_logo.svg)

---
# libmboxid

A Modbus TCP C++ library for unixoid systems.

## Overview

Modbus is an anachronistic fieldbus protocol. It was published by
Modicon (now Schneider Electric) in 1974, hence short after dinosaurs
became extinct. The protocol is standardized by the 
[Modbus organization](https://www.modbus.org) which provides free
access to the specification.

libmboxid implements Modbus TCP/IP. Future releases may support TLS.
Support for serial communication lines and/or a port to the Windows
operating system is not panned.

To get started, see [server.cpp](./examples/server.cpp) and
[client.cpp](./examples/client.cpp). The examples show how to implement Modbus server and
client applications with libmboxid.

## Installation instructions

### Building the library

```
$ cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/opt/mboxid
$ cmake --build build --target all
```

CMAKE\_INSTALL\_PREFIX defines the base directory in which the library
should be installed. If omitted, the prefix defaults to /usr/local.

### Running tests


```
$ ctest --test-dir ./build/tests/
```

### Installing the library

```
$ sudo cmake --build build --target install
```

or

```
$ sudo cmake --build build --target install/strip
```

### Cross compiling the library

If you use a Software Development Kit (SDK) built with the Yocto project,
cross compiling the is straight forward. Just source the SDK's environment file
before you build and install the library as described above.

For other toolchains please refer to
[Cross Compiling With CMake](https://cmake.org/cmake/help/book/mastering-cmake/chapter/Cross%20Compiling%20With%20CMake.html)

## Generating the documentation

Doxygen, Sphinx, Breathe and Exhale are required to generate the
documentation. Under Debian the required packages can be installed with:

```
$ sudo apt install doxygen python3-exhale
```

To generate the documentation:

```
$ cd doc
$ make html
```

to view the documentation:

```
$ firefox _build/html/index.html
```

and finally to remove the generated files:

```
$ make clean
```
