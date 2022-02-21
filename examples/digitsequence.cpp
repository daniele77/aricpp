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
#include <functional>
#include <boost/program_options.hpp>

#include "../include/aricpp/arimodel.h"
#include "../include/aricpp/bridge.h"
#include "../include/aricpp/channel.h"
#include "../include/aricpp/client.h"

using namespace aricpp;
using namespace std;

#if BOOST_VERSION < 106600
    using IoContext = boost::asio::io_service;
#else
    using IoContext = boost::asio::io_context;
#endif

//
class SequenceDetector
{
public:
    SequenceDetector(boost::asio::io_context& ios, std::chrono::milliseconds _timeout, function<void(string)>&& _invalidSeqHandler) :
        invalidSeqHandler(move(_invalidSeqHandler)),
        timeout(_timeout),
        timer(ios)
    {
        Reset();
    }
    void AddSequence(string&& sequence, function<void(string)>&& handler)
    {
        maxSeqSize = max(maxSeqSize, sequence.size());
        sequences.emplace_back(move(sequence), move(handler));
    }
    void Reset()
    {
        invalid = false;
        inputSequence.clear();
        StopTimer();
    }
    void StartDetection()
    {
        Reset();
        StartTimer();
    }
    bool Symbol(char s)
    {
        if (invalid) return false;

        inputSequence += s;
        for (auto& seq: sequences)
        {
            if (seq.first == inputSequence)
            {
                StopTimer();
                seq.second(inputSequence);
                return true;
            }
        }

        // check whether the inputSequence is invalid
        if (inputSequence.size() >= maxSeqSize)
            Invalid();

        return false;
    }
private:
    void StartTimer()
    {
        timer.expires_after(timeout);
        timer.async_wait(
            [this](boost::system::error_code e)
            {
                if (e) return;
                TimerExpired();
            });
    }
    void StopTimer()
    {
        timer.cancel();
    }
    void Invalid()
    {
        invalid = true;
        invalidSeqHandler(inputSequence);
    }
    void TimerExpired()
    {
        Invalid();
    }
private:
    using Sequence = pair<string, function<void(string)>>;
    vector<Sequence> sequences;
    function<void(string)> invalidSeqHandler;
    string inputSequence = {};
    size_t maxSeqSize = 0;
    bool invalid = false;
    const std::chrono::milliseconds timeout;
    boost::asio::steady_timer timer;
};

//

class Call
{
public:
    Call(IoContext& ios, shared_ptr<Channel> callingCh) :
        calling(std::move(callingCh)),
        sequenceDetector(ios, 10s, [this](string pin){ InvalidPin(pin); })
    {
        // here you can put all digit sequences and handlers
        sequenceDetector.AddSequence("777#", [this](string digits)
        {
            cout << "got " << digits << endl; 
            calling->Hangup(); 
        });
        sequenceDetector.AddSequence("8888#", [this](string digits)
        { 
            cout << "got " << digits << endl; 
            calling->StartMoh(); 
        });
    }

    bool HasChannel(const Channel& ch) const
    {
        return (calling->Id() == ch.Id());
    }

    void DialedChRinging()
    {
        // TODO
    }

    void Digit(const string& digits)
    {
        for (char d: digits)
            sequenceDetector.Symbol(d);
    }

    void DialedChStart()
    { 
        calling->Answer(); 
    }

    void PlaybackFinished()
    {
        sequenceDetector.StartDetection();
    }

    void DialingChUp()
    {
        calling->Play("sound:tt-monkeys");
    }

    bool ChHangup(const shared_ptr<Channel>& /*hung*/)
    {
        // TODO
        return true;
    }

private:

    void InvalidPin(const string& pin)
    {
        cout << "Wrong pin: " << pin << endl;
        calling->Hangup();
    }

    shared_ptr<Channel> calling;
    unique_ptr<Bridge> bridge;
    SequenceDetector sequenceDetector;
};

class CallContainer
{
public:
    CallContainer(IoContext& _ios, string app, AriModel& m, bool _sipCh) :
        application(move(app)),
        channels(m),
        chPrefix(CalcChPrefix(_sipCh)),
        ios(_ios)
    {
        channels.OnStasisStarted(
            [this](shared_ptr<Channel> ch, bool external)
            {
                if (external)
                    CallingChannel(ch);
                else
                    cerr << "Internal channel not allowed" << endl;
            });
        channels.OnChannelDestroyed(
            [this](shared_ptr<Channel> ch)
            {
                auto call = FindCallByChannel(ch);
                if (call)
                    Remove(call);
                else
                    cerr << "Call with a channel " << ch->Id() << " not found (hangup event)" << endl;
            });
        channels.OnChannelStateChanged(
            [this](shared_ptr<Channel> ch)
            {
                auto state = ch->GetState();
                if (state == Channel::State::ringing)
                {
                    auto call = FindCallByChannel(ch);
                    if (call)
                        call->DialedChRinging();
                    else
                        cerr << "Call with dialed ch id " << ch->Id() << " not found (ringing event)\n";
                }
                else if (state == Channel::State::up)
                {
                    auto call = FindCallByChannel(ch);
                    if (call) call->DialingChUp();
                }
            });
        channels.OnChannelDtmfReceived(
            [&](std::shared_ptr<aricpp::Channel> channel, const std::string& digit)
            {
                auto call = FindCallByChannel(channel);
                if (call) call->Digit(digit);
            });
        channels.OnPlaybackFinished(
            [this](Playback p)
            {
                auto channel = pb2ch[p.Id()];
                auto call = FindCallByChannel(channel);
                call->PlaybackFinished();
            }
        );

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

        calls.emplace_back(make_shared<Call>(ios, callingCh));
    }

    void Remove(const shared_ptr<Call>& call) { calls.erase(remove(calls.begin(), calls.end(), call), calls.end()); }

    // return empty shared_ptr if not found
    shared_ptr<Call> FindCallByChannel(const shared_ptr<Channel> ch) const
    {
        auto c = find_if(calls.begin(), calls.end(), [&](auto call) { return call->HasChannel(*ch); });
        return (c == calls.end() ? shared_ptr<Call>() : *c);
    }

    static string CalcChPrefix(bool sipCh) { return sipCh ? "sip/" : "pjsip/"; }

    const string application;
    vector<shared_ptr<Call>> calls;
    AriModel& channels;
    const string chPrefix;
    unordered_map<string, shared_ptr<Channel>> pb2ch;
    IoContext& ios;
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
            cout << "This is digits demo application v. 1.0, part of aricpp library\n";
            return 0;
        }

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
        CallContainer calls(ios, application, channels, sipCh);

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
