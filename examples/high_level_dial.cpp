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

#include <algorithm>
#include <exception>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <boost/program_options.hpp>

#include "../include/aricpp/arimodel.h"
#include "../include/aricpp/bridge.h"
#include "../include/aricpp/channel.h"
#include "../include/aricpp/client.h"

using namespace aricpp;
using namespace std;

enum class ChMode
{
    calling = 1,
    called = 2,
    both = 3
};

class Call
{
public:
    Call(AriModel& m, shared_ptr<Channel> callingCh, shared_ptr<Channel> calledCh, bool _moh)
        : model(m), calling(std::move(callingCh)), called(std::move(calledCh)), moh(_moh)
    {
    }

    bool HasChannel(const Channel& ch, ChMode mode) const
    {
        return (
            ((calling->Id() == ch.Id()) && (static_cast<int>(mode) & static_cast<int>(ChMode::calling))) ||
            ((called->Id() == ch.Id()) && (static_cast<int>(mode) & static_cast<int>(ChMode::called))));
    }

    void DialedChRinging()
    {
        if (moh)
            calling->StartMoh();
        else
            calling->Ring();
    }

    void DialedChStart() { calling->Answer(); }

    void DialingChUp()
    {
        model.CreateBridge(
            [this](unique_ptr<Bridge> newBridge)
            {
                if (!newBridge) return;

                bridge = move(newBridge);
                bridge->Add({&*calling, &*called});
            });
    }

    bool ChHangup(const shared_ptr<Channel>& hung)
    {
        shared_ptr<Channel> other = (hung->Id() == called->Id() ? calling : called);

        if (!other->IsDead()) other->Hangup();
        return (other->IsDead());
    }

private:
    AriModel& model;
    shared_ptr<Channel> calling;
    shared_ptr<Channel> called;
    unique_ptr<Bridge> bridge;
    bool moh;
};

class CallContainer
{
public:
    CallContainer(string app, AriModel& m, bool _moh, bool _autoAns, bool _sipCh) :
        application(move(app)),
        channels(m),
        moh(_moh),
        inviteVariables(CalcVariables(_autoAns, _sipCh)),
        chPrefix(CalcChPrefix(_sipCh))
    {
        channels.OnStasisStarted(
            [this](shared_ptr<Channel> ch, bool external)
            {
                if (external)
                    CallingChannel(ch);
                else
                    CalledChannel(ch);
            });
        channels.OnChannelDestroyed(
            [this](shared_ptr<Channel> ch)
            {
                auto call = FindCallByChannel(ch, ChMode::both);
                if (call)
                {
                    if (call->ChHangup(ch)) Remove(call);
                }
                else
                    cerr << "Call with a channel " << ch->Id() << " not found (hangup event)" << endl;
            });
        channels.OnChannelStateChanged(
            [this](shared_ptr<Channel> ch)
            {
                auto state = ch->GetState();
                if (state == Channel::State::ringing)
                {
                    auto call = FindCallByChannel(ch, ChMode::called);
                    if (call)
                        call->DialedChRinging();
                    else
                        cerr << "Call with dialed ch id " << ch->Id() << " not found (ringing event)\n";
                }
                else if (state == Channel::State::up)
                {
                    auto call = FindCallByChannel(ch, ChMode::calling);
                    if (call) call->DialingChUp();
                }
            });
    }
    CallContainer(const CallContainer&) = delete;
    CallContainer(CallContainer&&) = delete;
    CallContainer& operator=(const CallContainer&) = delete;
    CallContainer& operator=(CallContainer&&) = delete;

private:
    void CallingChannel(const shared_ptr<Channel>& callingCh)
    {
        const string callingId = callingCh->Id();
        const string name = callingCh->Name();
        const string ext = callingCh->Extension();
        const string callerNum = callingCh->CallerNum();
        string callerName = callingCh->CallerName();
        if (callerName.empty()) callerName = callerNum;

        callingCh->GetVar("CALLERID(all)")
            .OnError([](Error, const string& msg) { cerr << "Error retrieving variable CALLERID: " << msg << endl; })
            .After([](auto var) { cout << "CALLERID variable = " << var << endl; });

        auto calledCh = channels.CreateChannel();
        Create(callingCh, calledCh);
        calledCh->Dial(chPrefix + ext, application, callerName, inviteVariables)
            .OnError(
                [callingCh](Error e, const string& msg)
                {
                    if (e == Error::network)
                        cerr << "Error creating channel: " << msg << '\n';
                    else
                    {
                        cerr << "Error: reason " << msg << '\n';
                        callingCh->Hangup();
                    }
                })
            .After([]() { cout << "Call ok\n"; });
    }

    void CalledChannel(const shared_ptr<Channel>& calledCh)
    {
        auto call = FindCallByChannel(calledCh, ChMode::called);
        if (call)
            call->DialedChStart();
        else
            cerr << "Call with dialed ch id " << calledCh->Id() << " not found (stasis start event)\n";
    }

    void Create(shared_ptr<Channel> callingCh, shared_ptr<Channel> calledCh)
    {
        calls.emplace_back(make_shared<Call>(channels, callingCh, calledCh, moh));
    }

    void Remove(const shared_ptr<Call>& call) { calls.erase(remove(calls.begin(), calls.end(), call), calls.end()); }

    // return empty shared_ptr if not found
    shared_ptr<Call> FindCallByChannel(const shared_ptr<Channel> ch, ChMode mode) const
    {
        auto c = find_if(calls.begin(), calls.end(), [&](auto call) { return call->HasChannel(*ch, mode); });
        return (c == calls.end() ? shared_ptr<Call>() : *c);
    }

    static string CalcVariables(bool autoAns, bool sipCh)
    {
        if (!autoAns) return {};

        if (sipCh)
            return R"({"SIPADDHEADER0":"Call-Info:answer-after=0"})";
        else
            return "{\"PJSIP_HEADER(add,Call-info)\":\"answer-after=0\"}";
    }

    static string CalcChPrefix(bool sipCh) { return sipCh ? "sip/" : "pjsip/"; }

    const string application;
    vector<shared_ptr<Call>> calls;
    AriModel& channels;
    const bool moh;
    const string inviteVariables;
    const string chPrefix;
};

static std::string to_string(bool b) { return (b ? "true" : "false"); }

int main(int argc, char* argv[])
{
    try
    {
        string host = "localhost";
        string port = "8088";
        string username = "asterisk";
        string password = "asterisk";
        string application = "attendant";
        bool moh = false;
        bool autoAns = false;
        bool sipCh = false; // default = pjsip channel

        namespace po = boost::program_options;
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "produce help message")
            ("version,V", "print version string")

            ("host,H", po::value(&host), ("ip address of the ARI server ["s + host + ']').c_str())
            ("port,P", po::value(&port), ("port of the ARI server ["s + port + "]").c_str())
            ("username,u", po::value(&username), ("username of the ARI account on the server ["s + username + "]").c_str())
            ("password,p", po::value(&password), ("password of the ARI account on the server ["s + password + "]").c_str())
            ("application,a", po::value(&application), ("stasis application to use ["s + application + "]").c_str())
            ("moh,m", po::bool_switch(&moh), ("play music on hold instead of ringing ["s + to_string(moh) + "]").c_str())
            ("auto-answer,A", po::bool_switch(&autoAns), ("force the called endpoint to auto answer by using a custom header ["s + to_string(autoAns) + "]").c_str())
            ("sip-channel,S", po::bool_switch(&sipCh), ("use old sip channel instead of pjsip channel ["s + to_string(sipCh) + "]").c_str())
        ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            cout << desc << "\n";
            return 0;
        }

        if (vm.count("version"))
        {
            cout << "This is dial application v. 1.0, part of aricpp library\n";
            return 0;
        }

#if BOOST_VERSION < 106600
        using IoContext = boost::asio::io_service;
#else
        using IoContext = boost::asio::io_context;
#endif
        IoContext ios;

        // Register to handle the signals that indicate when the server should exit.
        // It is safe to register for the same signal multiple times in a program,
        // provided all registration for the specified signal is made through Asio.
        boost::asio::signal_set signals(ios);
        signals.add(SIGINT);
        signals.add(SIGTERM);
#if defined(SIGQUIT)
        signals.add(SIGQUIT);
#endif // defined(SIGQUIT)
        signals.async_wait(
            [&ios](boost::system::error_code /*ec*/, int /*signo*/)
            {
                cout << "Cleanup and exit application...\n";
                ios.stop();
            });

        Client client(ios, host, port, username, password, application);
        AriModel channels(client);
        CallContainer calls(application, channels, moh, autoAns, sipCh);

        client.Connect(
            [](boost::system::error_code e)
            {
                if (e)
                {
                    cerr << "Connection error: " << e.message() << endl;
                }
                else
                    cout << "Connected" << endl;
            },
            10s // reconnection seconds
        );
        ios.run();
    }
    catch (const exception& e)
    {
        cerr << "Exception in app: " << e.what() << ". Aborting\n";
        return -1;
    }
    return 0;
}
