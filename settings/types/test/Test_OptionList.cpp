/*
 * Copyright (c) 2015, The University of Oxford
 * All rights reserved.
 *
 * This file is part of the OSKAR package.
 * Contact: oskar at oerc.ox.ac.uk
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the University of Oxford nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <gtest/gtest.h>
#include <oskar_settings_types.hpp>

using namespace oskar;

TEST(settings_types, OptionList)
{
    OptionList l;
    ASSERT_TRUE(l.init("a,    b,"
                    ""
                    "c"));
    ASSERT_EQ(3u, l.options().size());
    ASSERT_EQ(3, l.size());
    ASSERT_STREQ("a", l.options()[0].c_str());
    ASSERT_STREQ("b", l.options()[1].c_str());
    ASSERT_STREQ("c", l.options()[2].c_str());
    ASSERT_STREQ("", l.get_value().c_str());
    ASSERT_TRUE(l.is_default());
    ASSERT_FALSE(l.set_default("z"));
    ASSERT_STREQ("", l.get_default().c_str());
    ASSERT_TRUE(l.set_default("b"));
    ASSERT_STREQ("b", l.get_default().c_str());
    ASSERT_TRUE(l.set_value("a"));
    ASSERT_FALSE(l.is_default());
    ASSERT_STREQ("a", l.get_value().c_str());
}

