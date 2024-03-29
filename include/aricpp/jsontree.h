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

#ifndef ARICPP_JSONTREE_H_
#define ARICPP_JSONTREE_H_

#include <iostream>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

namespace aricpp
{

// Json encapsulation

using JsonTree = boost::property_tree::ptree;

inline void Dump(const JsonTree& e) { boost::property_tree::write_json(std::cout, e); }
inline std::string ToString(const JsonTree& e)
{
    std::ostringstream os;
    boost::property_tree::write_json(os, e);
    return os.str();
}

inline JsonTree FromJson(const std::string& s)
{
    boost::property_tree::ptree tree;
    std::stringstream ss;
    ss << s;
    boost::property_tree::read_json(ss, tree);
    return tree;
}

template<typename T>
T Get(const JsonTree& e, const std::vector<std::string>& path)
{
    assert(!path.empty());
    std::string s;
    for (const auto& piece : path)
        s += piece + '.';
    s.pop_back();
    return e.get<T>(s);
}

template<>
inline std::vector<std::string> Get(const JsonTree& e, const std::vector<std::string>& path)
{
    assert(!path.empty());
    std::string s;
    for (const auto& piece : path)
        s += piece + '.';
    s.pop_back();

    std::vector<std::string> result;
    const auto& args = e.get_child(s);
    for (auto& child : args)
        result.push_back(child.second.get_value<std::string>());

    return result;
}

} // namespace aricpp

#endif // ARICPP_JSONTREE_H_
