/*******************************************************************************
 * ARICPP - ARI interface for C++
 * Copyright (C) 2017 Daniele Pallastrelli
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


#ifndef ARICPP_PROXY_H_
#define ARICPP_PROXY_H_

#include <string>
#include <memory>
#include "client.h"
#include "errors.h"
#include "method.h"

namespace aricpp
{

class Proxy
{
public:
    using ErrorHandler = std::function<void(Error,const std::string&)>;
    using AfterHandler = std::function<void(void)>;

    Proxy& After(AfterHandler f)
    {
        if ( afterHandler ) // sequence of std::function
        {
            auto g = afterHandler;
            afterHandler = [g,f](){ g(); f(); };
        }
        else
            afterHandler = f;
        return *this;
    }
    Proxy& OnError(ErrorHandler f)
    {
        if ( errorHandler ) // sequence of std::function
        {
            auto g = errorHandler;
            errorHandler = [g,f](Error e, const std::string& msg){ g(e, msg); f(e, msg); };
        }
        else
            errorHandler = f;
        return *this;
    }

private:
    friend class Channel;
    friend class Bridge;

    void SetError(Error e, const std::string& msg)
    {
        if (errorHandler) errorHandler(e, msg);
    }
    void Completed()
    {
        if (afterHandler) afterHandler();
    }

    static Proxy& CreateEmpty()
    {
        return *std::make_shared<Proxy>();
    }

    static Proxy& Command(Method method, std::string request, Client* client, std::string body={})
    {
        auto proxy = std::make_shared<Proxy>();
        client->RawCmd(
            method,
            std::move(request),
            [proxy](auto e, int state, auto reason, auto)
            {
                if (e)
                    proxy->SetError(Error::network, e.message());
                else
                {
                    if (state/100 == 2)
                        proxy->Completed();
                    else
                        proxy->SetError(Error::unknown, reason);
                }
            },
            std::move(body)
        );
        return *proxy;
    }

    ErrorHandler errorHandler;
    AfterHandler afterHandler;
};

} // namespace aricpp

#endif
