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

#ifndef ARICPP_RECORDING_H_
#define ARICPP_RECORDING_H_

#include <string>
#include <utility>
#include "client.h"
#include "proxy.h"

namespace aricpp
{

class Recording
{
public:
    Recording() = default;
    Recording(const Recording&) = default;
    Recording(Recording&&) = default;
    Recording& operator=(const Recording&) = default;
    Recording& operator=(Recording&&) = default;

    Proxy Delete()
    {
        if (name.empty()) return Proxy::CreateEmpty();
        return Proxy::Command(Method::delete_, "/ari/recordings/stored/" + name, client);
    }

    Proxy Abort()
    {
        if (name.empty()) return Proxy::CreateEmpty();
        return Proxy::Command(Method::delete_, "/ari/recordings/live/" + name, client);
    }

    Proxy Stop()
    {
        if (name.empty()) return Proxy::CreateEmpty();
        return Proxy::Command(Method::post, "/ari/recordings/live/" + name + "/stop", client);
    }

    Proxy Mute()
    {
        if (name.empty()) return Proxy::CreateEmpty();
        return Proxy::Command(Method::post, "/ari/recordings/live/" + name + "/mute", client);
    }

    Proxy Unmute()
    {
        if (name.empty()) return Proxy::CreateEmpty();
        return Proxy::Command(Method::delete_, "/ari/recordings/live/" + name + "/stop", client);
    }

    Proxy Pause()
    {
        if (name.empty()) return Proxy::CreateEmpty();
        return Proxy::Command(Method::post, "/ari/recordings/live/" + name + "/pause", client);
    }

    Proxy Resume()
    {
        if (name.empty()) return Proxy::CreateEmpty();
        return Proxy::Command(Method::delete_, "/ari/recordings/live/" + name + "/pause", client);
    }

private:
    friend class Channel;
    friend class Bridge;

    Recording(std::string _name, Client* _client) : name(std::move(_name)), client(_client) {}

    std::string name;
    Client* client;
};

} // namespace aricpp

#endif
