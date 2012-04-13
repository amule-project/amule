aMule with Boost ASIO networking - by Stu Redman

We are still having some networking problems with wxWidgets, even with 2.8.12.
With wx 2.9 SVN networking was completely broken for some versions. So I wanted to see
how a different networking layer behaves, and if it is even possible to integrate
in aMule.
I decided on Boost ASIO because of the good reputation of the Boost library.

The goal is to have an aMule which can be configured to use either wx Sockets
or ASIO sockets for all major tasks (this may exclude things like http download).
And with ASIO sockets it should perform better at least in some aspects 
(stability, speed, CPU, memory) for this endeavour to make sense.

There are no plans to "boostify" aMule. Boost is encapsulated in the implementation
and not even included in the headers.

Implementation is near complete. It's been tested in Windows and Linux.
More testing is required.

In some environments using Boost saves a lot of CPU, in others it doesn't.

Not done yet:
- better error handling/error messages
- further abstraction (replace wx socket constants in code)
- make use of background resolve for AsyncDNS instead of spawning a thread

If you like to play with new stuff I'd like you to try it out and give me feedback.
Especially if you are having networking problems or out of memory crashes, or if you
are running aMule at high speeds.


Compilation:

Get current boost lib from www.boost.org . Extract it to your favorite 
place (like ~/amule/boost_1_49_0).
Boost.Asio is header-only. It requires Boost.System for its error codes,
but we include the single .cpp directly, so we don't need to build
anything of boost.

Configure aMule using
  ./configure --with-boost
or
  ./configure --with-boost=/path/to/boost/sources

Important: aMule prints "Asio thread started" in log and console on startup. 
If it doesn't you have probably configured it to use wx sockets!
