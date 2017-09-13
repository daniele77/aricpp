# aricpp
Asterisk ARI interface bindings for modern C++


## Properties

* modern C++
* multiplatform
* header only
* asynchronous
* high level interface to manipulate asterisk concepts (channels, bridges, ...)
* low level interface to send and receive ARI commands and events from asterisk
* fluent interface
* efficient (tested with sipp traffic generator)


## Requirements

aricpp requires a c++14 compiler, and relies on the following libraries:
* boost (www.boost.org)
* beast (http://github.com/boostorg/beast tested with version 108)


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
aricpp, boost and beast header files and linking boost system, boost program options
(and pthread on linux).

Are also provided:
* a GNU make file (Makefile)
* a Windows nmake file (makefile.win)
* a Visual Studio solution

You can specify boost and beast library paths in the following ways:

### GNU Make
    
    make CXXFLAGS="-I<boost_include> -isystem <beast_include>" LDFLAGS=-L<boost_lib>

example:

    make CXXFLAGS="-I/opt/boost_1_63_0/install/x86/include -isystem /opt/beast/include/" LDFLAGS=-L/opt/boost_1_63_0/install/x86/lib

(if you want to use clang instead of gcc, you can set the variable CXX=clang++) 
 
### Windows nmake

Set the environment variables BOOST and BEAST. Then, from a visual studio console, use the command:

    nmake /f makefile.win
	
### Visual Studio solution

Set the environment variables BOOST and BEAST. Then, open the file
`aricpp/samples/aricpp_samples/aricpp_samples.sln`


## Compilation of the Doxygen documentation

If you have doxygen installed on your system, you can get the html documentation
of the library in this way:

    <enter the directory doc>
    doxygen Doxyfile
    

## Five minutes tutorial

aricpp is based on boost asio async library. So, to use it you should have an instance
of `boost::asio::io_service` running and an instance of `aricpp::Client`:

```C++
#include "aricpp/client.h"

...
boost::asio::io_service ios;
aricpp::Client client(ios, host, port, username, password, stasisapp);
...
ios.run();
```

To connect to the server use the method `aricpp::Client::Connect`:

```C++
aricpp::Client client(ios, host, port, username, password, stasisapp);
client.Connect( [](boost::system::error_code e){
    if (e) cerr << "Error connecting: " << e.message() << '\n';
    else cout << "Connected" << '\n';
});
```

Once connected, you can register for raw events by using the method `aricpp::Client::OnEvent`:

```C++
client.Connect( [&](boost::system::error_code e){
    if (e)
    {
        cerr << "Error connecting: " << e.message() << '\n';
        return;
    }
    cout << "Connected" << '\n';
    client.OnEvent("StasisStart", [](const Event& e){
        Dump(e); // print the json on the console
        auto id = Get<string>(e, {"channel", "id"});
        cout << "Channel id " << id << " entered stasis application\n";
    });
});
```

Finally, you can send requests by using the method `aricpp::Client::RawCmd`:

```C++
client.RawCmd(
    "GET",
    "/ari/channels",
    [](boost::system::error_code error, int state, string reason, string body)
    {
        // if no errors, body contains the detail of all channels
    }
);
```

aricpp also provides a higher level interface, with which you can manipulate
asterisk telephonic objects (e.g., channels).

To use this interface, you need to create an instance of the class `AriModel`,
on which you can register for channel events (`AriModel::OnStasisStarted`, 
`AriModel::OnStasisDestroyed`, `AriModel::OnChannelStateChanged`) and
create channels (`AriModel::CreateChannel()`).

All these methods give you references to `Channel` objects, that provide the methods
for the usual actions on asterisk channels (e.g., ring, answer, hangup, dial, ...).

```C++
boost::asio::io_service ios;
aricpp::Client client(ios, host, port, username, password, stasisapp);
AriModel channels( client );

client.Connect( [&](boost::system::error_code e){
	if (e)
	{
		cerr << "Connection error: " << e.message() << endl;
		ios.stop();
	}
	else
	{
		cout << "Connected" << endl;
		
		channels.OnStasisStarted(
            [this](shared_ptr<Channel> ch, bool external)
            {
                if (external) CallingChannel(ch);
                else CalledChannel(ch);
            }
        );
		
        auto ch = channels.CreateChannel();
        ch->Call("pjsip/100", stasisapp, "caller name")
            .OnError([callingCh](Error e, const string& msg)
                {
                    if (e == Error::network)
                        cerr << "Error creating channel: " << msg << '\n';
                    else
                    {
                        cerr << "Error: reason " << msg << '\n';
                    }
                }
            )
            .After([]() { cout << "Call ok\n"; } );
	}
});
...
ios.run();
```

See the sample `high_level_dial.cpp` (located in directory `samples`) to have a
full working example.

The high and low level interface can coexist. Being the high-level interface still
under development, you can use the low-level interface for the missing commands.

