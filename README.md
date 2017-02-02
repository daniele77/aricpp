# aricpp
Asterisk ARI interface bindings for modern C++


## Requirements

aricpp relies on the following libraries:
* boost (www.boost.org)
* beast (http://github.com/vinniefalco/Beast)


## Installation

aricpp library is header-only: it consists entirely of header files
containing templates and inline functions, and require no separately-compiled
library binaries or special treatment when linking.
 
Extract the archive wherever you want.

Now you must only remember to insert the aricpp, beast and boost path when
compiling your source code.


## Compilation of the samples

You can find some examples in the directory "samples".
Each .cpp files correspond to an executable. You can compile each sample by including
aricpp, boost and beast header files and linking boost system, boost program options and pthread.

There are also a GNU make file (Makefile) and a Windows nmake file (makefile.win).

You can specify boost and beast library paths in the following ways:

* GNU Make
        make CXXFLAGS=-I<boost_path> CXXFLAGS="-I<boost_include> -isystem <beast_include>" LDFLAGS=-L<boost_lib>
    example:
        make CXXFLAGS="-I/opt/boost_1_63_0/install/x86/include -isystem /opt/beast/include/" LDFLAGS=-L/opt/boost_1_63_0/install/x86/lib
    (if you want to use clang instead of gcc, you can set the variable CXX=clang++) 
 
* Windows nmake
    Set the environment variable BOOST.
    Then, from a visual studio console, use the command:
    nmake /f makefile.win


## Compilation of the Doxygen documentation

If you have doxygen installed on your system, you can get the html documentation
of the library in this way:
     <enter the directory doc>
     doxygen Doxyfile
