User guide
==========

This chapter provides a short tour through libmodbus.

Introduction
------------

Modbus is an anachronistic fieldbus protocol. It was published by
Modicon (now Schneider Electric) in 1974, hence short after dinosaurs
became extinct. The protocol is standardized by the
`Modbus organization <https://www.modbus.org>`_ which provides free
access to the specification.

libmboxid implements Modbus TCP/IP. Future releases may support TLS.
Support for serial communication lines and/or a port to the Windows
operating system is not panned.

Quick start
-----------

Get the library from
`GitHub <https://github.com/mboxid/libmboxid>`_
and follow the instructions there to build and install it.

Take a look at :ref:`section_server_example` and
:ref:`section_client_example` to see how to implement a Modbus
TCP server and client respectively.

Other implementations
---------------------

Consider the
`FieldTalk C++ Modbus Libraries <https://www.modbusdriver.com/fieldtalk>`_
if you are looking for commercial products with adequate customer support.
The concept of a backend connector class was inspired by their Modbus server
library.

If you prefer open source software `libmodbus <https://libmodbus.org>`_
might be a great choice. The library is ideal for implementing
clients, i.e. for communicating with Modbus devices that act as server.
The library has some flaws when it comes to the implementation of servers:

- The library imposes multiplexing of clients on the application.
- The library provides tables according to the specification, but lacks means
  to connect them to the application. It misses a reasonable backend interface.

The shortcomings of libmodbus for use as Modbus server were the motivation
to start with our own open source Modbus TCP/IP C++ library.

Data model
----------

Modbus defines its data model based on a series of tables. The four primary
tables are coils, discrete inputs, input registers and holding registers
[#Modbus_Wiki]_.

The data types constructing the tables have the following characteristics:

================    ==========  ======  ==============================
Data type           Access      Size    Example of use
================    ==========  ======  ==============================
Coil                Read-write  1 bit   Digital output (with read-back)
Discrete input      Read-only   1 bit   Digital input
Input register      Read-only   16 bit  Measurements and statuses
Holding register    Read-write  16 bit  Configuration values
================    ==========  ======  ==============================

The terms are somehow anachronistic. However, to avoid confusion libmboxid
adheres to the terms used in the Modbus specification. The terms are coined
by early industrial control systems: A single-bit physical output
was usually implemented by driving a relay, and therefore called coil.
A single-bit physical input is called a discrete input or a contact
[#Modbus_Wiki]_.

Supported function codes
------------------------

The following function codes are supported by libmboxid:

===========     =============================   ====================
Code            Description                     Notes
===========     =============================   ====================
01              Read coils
02              Read discrete inputs
03              Read holding registers
04              Read input registers
05              Write single coil
06              Write single register
15              Write multiple coils
16              Write multiple registers
22              Mask write register
23              Read/write multiple registers
43, MEI: 14     Read device identification      Basic w/ limitations
===========     =============================   ====================

Timeouts
--------

Timeouts are specified in [millisecond] as this is the resolution used by
the underlying system calls. However, as a big fat warning: Don not select
timeouts that are too short, as otherwise the mechanisms in TCP that ensure
reliable communication will be ineffective. Response timeouts should be at
least several seconds, connect timeouts in the range of minutes.

Logging
-------

The library logs important events to stdout/stderr by default. Someone can
provide their own sink by installing a customized logger class
derived from :class:`mboxid::log::logger_base`.

Modbus TCP server
-----------------

Follow the steps below to write your own Modbus TCP server application:

1. Implement a :class:`mboxid::backend_connector`. \
   The class connects the Modbus server with the user application.
2. Create an instance of :class:`mboxid::modbus_tcp_server`.
3. Configure the server instance:

    * :func:`mboxid::modbus_tcp_server::set_backend`
    * :func:`mboxid::modbus_tcp_server::set_server_addr` (optional)
    * :func:`mboxid::modbus_tcp_server::set_idle_timeout` (optional)
    * :func:`mboxid::modbus_tcp_server::set_request_complete_timeout` (optional)

4. Execute :func:`mboxid::modbus_tcp_server::run`
5. Stop the server by a call to :func:`mboxid::modbus_tcp_server::shutdown`

Thread safety
^^^^^^^^^^^^^

Most methods of :class:`mboxid::modbus_tcp_server` are designed to be called
in sequence. They are not thread-safe. However, it is safe to execute
:func:`mboxid::modbus_tcp_server::run` in its own thread context and call
:func:`mboxid::modbus_tcp_server::shutdown` from a different thread.
Thread-safe methods are explicitly labeled in the API documentation.


.. _section_server_example:

Server example
^^^^^^^^^^^^^^

This section provides an example of an Modbus TCP server application.

.. literalinclude:: ../examples/server.cpp
    :language: cpp
    :linenos:

Modbus TCP client
-----------------

Follow the steps below to write your own Modbus TCP client application:

1. Create an instance of :class:`mboxid::modbus_tcp_client`.
2. Connect to the server by calling \
   :func:`mboxid::modbus_tcp_client::connect_to_server`.
3. Set time limit for responses with \
   :func:`mboxid::modbus_tcp_client::set_response_timeout`.
4. Read/write from/to the server:

    * :func:`mboxid::modbus_tcp_client::read_coils`
    * :func:`mboxid::modbus_tcp_client::read_discrete_inputs`
    * :func:`mboxid::modbus_tcp_client::write_single_coil`
    * etc.

5. Use :func:`mboxid::modbus_tcp_client::disconnect` to disconnect from
   the server.

Thread safety
^^^^^^^^^^^^^

A particular :class:`mboxid::modbus_tcp_client` instance is not thread-safe
on its own. However, it is safe to have several client instances scattered
across threads within the same application.


.. _section_client_example:

Client example
^^^^^^^^^^^^^^

This section provides an example of an Modbus TCP client application.

.. literalinclude:: ../examples/client.cpp
    :language: cpp
    :linenos:

.. rubric:: Footnotes

.. [#Modbus_Wiki] https://en.wikipedia.org/wiki/Modbus
