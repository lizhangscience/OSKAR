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

#include <ttl/var/variant.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <climits>
#include <vector>
#include <oskar_settings_utility_string.hpp>
#include <oskar_SettingsValue.hpp>
#include <oskar_settings_types.hpp>

using namespace std;
using namespace oskar;

TEST(SettingsValue, test1)
{
    SettingsValue v;
    ASSERT_EQ(SettingsValue::UNDEF, v.type());
    ASSERT_STREQ("Undef", v.type_name().c_str());

    Bool b;
    ASSERT_TRUE(b.set_default("false"));

    v = Bool();
    ASSERT_EQ(SettingsValue::BOOL, v.type());
    ASSERT_TRUE(v.is_default());
    ASSERT_TRUE(v.get<Bool>().set_default("false"));
    ASSERT_TRUE(v.get<Bool>().set_value("true"));
    ASSERT_FALSE(v.is_default());
    ASSERT_STREQ("true", v.get<Bool>().get_value().c_str());
    ASSERT_TRUE(v.set<Bool>("false"));
    ASSERT_TRUE(v.is_default());
    ASSERT_STREQ("false", v.get<Bool>().get_value().c_str());
    ASSERT_STREQ("false", v.value<Bool>().c_str());
    ASSERT_STREQ("false", v.get_value().c_str());

    ASSERT_TRUE(v.init("DateTime", ""));
    ASSERT_EQ(SettingsValue::DATE_TIME, v.type());
    ASSERT_TRUE(v.set<DateTime>("1985-5-23T5:6:12.12345"));
    ASSERT_EQ(1985, v.get<DateTime>().value().year);
    ASSERT_EQ(DateTime::ISO, v.get<DateTime>().value().style);
    ASSERT_EQ(std::string(), v.get<DateTime>().get_default());
    ASSERT_STREQ("1985-05-23T05:06:12.12345", v.get_value().c_str());

    ASSERT_TRUE(v.init("Double", ""));
    ASSERT_TRUE(v.set_default("2.0"));
    ASSERT_EQ(SettingsValue::DOUBLE, v.type());
    ASSERT_STREQ("Double", v.type_name().c_str());
}

