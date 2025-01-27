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

#ifndef ARICPP_ARIMODEL_H_
#define ARICPP_ARIMODEL_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include "bridge.h"
#include "channel.h"
#include "client.h"

namespace aricpp
{

/**
 * @brief Provides the telephony object model on top of a asterisk connection.
 *
 * This class provides methods to create channels (class Channel) and
 * register handlers for stasis and channel events.
 */
class AriModel
{
public:
    using ChannelPtr = std::shared_ptr<Channel>;

    explicit AriModel(Client& c) : client(c) { Subscribe(); }

    AriModel(const AriModel&) = delete;
    AriModel(const AriModel&&) = delete;
    AriModel& operator=(const AriModel&) = delete;
    AriModel& operator=(const AriModel&&) = delete;

    using ChHandler = std::function<void(ChannelPtr)>;
    using ChVarSetHandler = std::function<void(ChannelPtr, const std::string&, const std::string&)>;
    using ChToneDetectedHandler = std::function<void(ChannelPtr)>;
    using ChDtmfHandler = std::function<void(ChannelPtr, const std::string&)>;
    using PlaybackHandler = std::function<void(Playback)>;
    using TextMessageHandler = std::function<void(const std::string& from, const std::string& to, const std::string& msg)>;
    using StasisStartedHandler = std::function<void(ChannelPtr, bool external)>;

    void OnChannelCreated(ChHandler handler) { chCreated = std::move(handler); }
    void OnChannelDestroyed(ChHandler handler) { chDestroyed = std::move(handler); }
    void OnChannelStateChanged(ChHandler handler) { chStateChanged = std::move(handler); }
    void OnChannelVarSet(ChVarSetHandler handler) { chVarSet = std::move(handler); }
    void OnChannelToneDetected(ChToneDetectedHandler handler) { chToneDetected = std::move(handler); }
    void OnChannelDtmfReceived(ChDtmfHandler handler) { chDtmfReceived = std::move(handler); }
    void OnStasisStarted(StasisStartedHandler handler) { stasisStarted = std::move(handler); }
    void OnPlaybackStarted(PlaybackHandler handler) { playbackStarted = std::move(handler); }
    void OnPlaybackFinished(PlaybackHandler handler) { playbackFinished = std::move(handler); }
    void OnTextMessageReceived(TextMessageHandler handler) { textMessageReceived = std::move(handler); }

    ChannelPtr CreateChannel()
    {
        // generate an id for the new channel
        const std::string id = "aricpp-c" + std::to_string(nextId++);
        auto it = channels.emplace(id, std::make_shared<Channel>(client, id));
        return it.first->second;
    }

    /// Create a new bridge on asterisk
    /// The handler takes a unique_ptr<Bridge> to the new bridge as parameter
    /// If an error occour, the unique_ptr passed is empty.
    template<typename CreationHandler>
    void CreateBridge(CreationHandler&& h, const Bridge::Type& type = Bridge::Type::mixing)
    {
        client.RawCmd(
            Method::post,
            "/ari/bridges?type=" + static_cast<std::string>(type),
            [this, h(std::forward<CreationHandler>(h))](auto, auto, auto, auto body)
            {
                try
                {
                    auto tree = FromJson(body);
                    /*
                    auto bridge = std::make_unique<Bridge>(
                        Get<std::string>(tree, {"id"}),
                        Get<std::string>(tree, {"technology"}),
                        Get<std::string>(tree, {"bridge_type"}),
                        &_client
                    );
                    h(std::move(bridge));
                    */
                    auto* bridge = new Bridge(
                        Get<std::string>(tree, {"id"}),
                        Get<std::string>(tree, {"technology"}),
                        Get<std::string>(tree, {"bridge_type"}),
                        &client
                    );
                    h(std::unique_ptr<Bridge>(bridge));
                }
                catch (const std::exception& e)
                {
                    // TODO: remove print?
                    std::cerr << "Exception in POST bridge response: " << e.what() << '\n';
                    h({});
                }
                catch (...)
                {
                    // TODO: remove print?
                    std::cerr << "Unknown exception in POST bridge response\n";
                    h({});
                }
            });
    }

    Proxy SendTextMessage(const std::string& from, const std::string& to, const std::string& msg)
    {
        return Proxy::Command(
            Method::put,
            "/ari/endpoints/sendMessage"
                "?to=" + to + 
                "&from=" + from +
                "&body=" + UrlEncode(msg),
            &client
        );
    }

private:
    void Subscribe()
    {
        client.OnEvent(
            "ChannelCreated",
            [this](const JsonTree& e)
            {
                auto state = Get<std::string>(e, {"channel", "state"});
                auto id = Get<std::string>(e, {"channel", "id"});
                auto findResult = channels.find(id);
                ChannelPtr channel;
                if (findResult == channels.end())
                {
                    auto it = channels.emplace(id, std::make_shared<Channel>(client, id, state));
                    channel = it.first->second;
                }
                else
                {
                    channel = findResult->second;
                    channel->StateChanged(state);
                }
                if (chCreated) chCreated(channel);
            });

        client.OnEvent(
            "StasisStart",
            [this](const JsonTree& e)
            {
                const std::string id = Get<std::string>(e, {"channel", "id"});
                const std::string name = Get<std::string>(e, {"channel", "name"});
                const std::string ext = Get<std::string>(e, {"channel", "dialplan", "exten"});
                const std::string callerNum = Get<std::string>(e, {"channel", "caller", "number"});
                const std::string callerName = Get<std::string>(e, {"channel", "caller", "name"});
                const auto& args = Get<std::vector<std::string>>(e, {"args"});

                auto ch = channels.find(id);
                if (ch == channels.end()) return;

                ch->second->StasisStart(name, ext, callerNum, callerName);
                if (stasisStarted) stasisStarted(ch->second, args.empty());
            });
        client.OnEvent(
            "ChannelDestroyed",
            [this](const JsonTree& e)
            {
                auto id = Get<std::string>(e, {"channel", "id"});
                auto cause = Get<int>(e, {"cause"});
                auto causeTxt = Get<std::string>(e, {"cause_txt"});

                auto ch = channels.find(id);
                if (ch == channels.end()) return;
                ch->second->Dead(cause, causeTxt);
                if (chDestroyed) chDestroyed(ch->second);
                channels.erase(id);
            });
        client.OnEvent(
            "ChannelStateChange",
            [this](const JsonTree& e)
            {
                auto id = Get<std::string>(e, {"channel", "id"});
                auto state = Get<std::string>(e, {"channel", "state"});
                auto ch = channels.find(id);
                if (ch == channels.end()) return;
                ch->second->StateChanged(state);
                if (chStateChanged) chStateChanged(ch->second);
            });

        client.OnEvent(
            "ChannelVarset",
            [this](const JsonTree& e)
            {
                if (!chVarSet) return;

                auto variable = Get<std::string>(e, {"variable"});
                auto value = Get<std::string>(e, {"value"});
                try
                {
                    auto chId = Get<std::string>(e, {"channel", "id"});
                    auto ch = channels.find(chId);
                    if (ch == channels.end()) return;
                    // channel variable
                    chVarSet(ch->second, variable, value);
                }
                catch (std::exception&)
                {
                    // global variable
                    chVarSet(nullptr, variable, value);
                }
            });

        client.OnEvent(
            "ChannelToneDetected",
            [this](const JsonTree& e)
            {
                if (!chToneDetected) return;

                auto id = Get<std::string>(e, {"channel", "id"});
                auto ch = channels.find(id);

                if (ch == channels.end()) return;

                chToneDetected(ch->second);
            });

        client.OnEvent(
            "ChannelDtmfReceived",
            [this](const JsonTree& e)
            {
                if (!chDtmfReceived) return;
                auto id = Get<std::string>(e, {"channel", "id"});
                auto digit = Get<std::string>(e, {"digit"});
                auto ch = channels.find(id);
                if (ch == channels.end()) return;
                chDtmfReceived(ch->second, digit);
            });

        client.OnEvent(
            "PlaybackStarted",
            [this](const JsonTree& e)
            {
                const std::string id = Get<std::string>(e, {"playback", "id"});
                const std::string media_uri = Get<std::string>(e, {"playback", "media_uri"});
                const std::string target_uri = Get<std::string>(e, {"playback", "target_uri"});
                const std::string language = Get<std::string>(e, {"playback", "language"});
                const std::string state = Get<std::string>(e, {"playback", "state"});
                if (playbackStarted) playbackStarted(Playback(id, &client));
            });

        client.OnEvent(
            "PlaybackFinished",
            [this](const JsonTree& e)
            {
                const std::string id = Get<std::string>(e, {"playback", "id"});
                const std::string media_uri = Get<std::string>(e, {"playback", "media_uri"});
                const std::string target_uri = Get<std::string>(e, {"playback", "target_uri"});
                const std::string language = Get<std::string>(e, {"playback", "language"});
                const std::string state = Get<std::string>(e, {"playback", "state"});
                if (playbackFinished) playbackFinished(Playback(id, &client));
            });

        client.OnEvent(
            "TextMessageReceived",
            [this](const JsonTree& e)
            {
                const std::string from = Get<std::string>(e, {"message", "from"});
                const std::string to = Get<std::string>(e, {"message", "to"});
                const std::string msg = Get<std::string>(e, {"message", "body"});
                if (textMessageReceived) textMessageReceived(from, to, msg);
            });
    }

    Client& client;
    std::unordered_map<std::string, ChannelPtr> channels;
    unsigned long long nextId = 0;

    ChHandler chCreated;
    ChHandler chDestroyed;
    ChHandler chStateChanged;
    ChVarSetHandler chVarSet;
    ChToneDetectedHandler chToneDetected;
    ChDtmfHandler chDtmfReceived;
    StasisStartedHandler stasisStarted;
    PlaybackHandler playbackStarted;
    PlaybackHandler playbackFinished;
    TextMessageHandler textMessageReceived;
};

} // namespace aricpp

#endif
