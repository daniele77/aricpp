/*******************************************************************************
 * ARICPP - ARI interface for C++
 * Copyright (C) 2017-2021 Daniele Pallastrelli
 *
 * This file is part of aricpp.
 * For more information, see http://github.com/daniele77/aricpp
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************/


#include <boost/program_options.hpp>
#include <string>
#include <thread>
#include "../include/aricpp/client.h"
#include "../include/aricpp/arimodel.h"

using namespace std;
using namespace aricpp;

int main( int argc, char* argv[] )
{
    try
    {
        string host = "localhost";
        string port = "8088";
        string username = "asterisk";
        string password = "asterisk";
        string application = "attendant";
        string to = "291";
        string from = "290";
        string tech = "pjsip";

        namespace po = boost::program_options;
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "produce help message")
            ("version,V", "print version string")

            ("host,H", po::value(&host), "ip address of the ARI server [localhost]")
            ("port,P", po::value(&port), "port of the ARI server [8088]")
            ("username,u", po::value(&username), "username of the ARI account on the server [asterisk]")
            ("password,p", po::value(&password), "password of the ARI account on the server [asterisk]")
            ("application,a", po::value(&application), "stasis application to use [attendant]")
            ("technology,T", po::value(&tech), "technology to use for the endpoints (pjsip, sip, xmpp) [pjsip]")
            ("from,f", po::value(&from), "source extension [290]")
            ("to,t", po::value(&to), "destination extension [291]")
        ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cout << desc << "\n";
            return 0;
        }

        if (vm.count("version"))
        {
            cout << "This is chat application v. 1.0, part of aricpp library\n";
            return 0;
        }

#if BOOST_VERSION < 106600
        using IoContext = boost::asio::io_service;
#else
        using IoContext = boost::asio::io_context;
#endif
        IoContext ios;

        Client client( ios, host, port, username, password, application );
        AriModel model(client);
        model.OnTextMessageReceived([](const std::string& src, const std::string& dest, const std::string& msg){
            cout << "Message from: " << src << " (for: " << dest << "):\n";
            cout << msg << "\n\n";
        });
        client.Connect( [&](boost::system::error_code e){
            if (e)
            {
                std::cerr << "Cannot connect to asterisk. Aborting.\n";
                exit(EXIT_FAILURE);
            }
        });

        auto inputReader = [&]()
        {
            string msg;
            while (true)
            {
                cout << "Enter you message (quit to exit):\n> ";
                getline( std::cin, msg );
                if (msg == "exit" || msg == "quit") break;
                if (msg.empty()) continue;
                cout << "Sending " << msg << '\n';
                auto sendRequest =
                    [&model,from, to, msg, &tech]()
                    {
                        model.SendTextMsg(tech + ':' + from, tech + ':' + to, msg)
                             .OnError([](Error /*e*/, const string& errMsg){ std::cerr << "Error: " << errMsg << std::endl; });
                    };

#if BOOST_VERSION < 106600
                ios.post(sendRequest);
#else
                boost::asio::post(ios.get_executor(), sendRequest);
#endif
            }
            cout << "Exiting application\n";
            ios.stop();
        };
        thread readerThread( inputReader );

        ios.run();

        readerThread.join();

    }
    catch ( const exception& e )
    {
        cerr << "Exception in app: " << e.what() << ". Aborting\n";
        return -1;
    }
    return 0;
}
