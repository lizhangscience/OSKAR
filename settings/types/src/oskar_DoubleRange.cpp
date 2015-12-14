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

#include <oskar_settings_utility_string.hpp>

#include <cfloat>
#include <vector>
#include <cmath>
#include <cfloat>
#include <iostream>
#include <oskar_DoubleRange.hpp>

namespace oskar {

DoubleRange::DoubleRange()
: format_(AUTO), min_(-DBL_MAX), max_(DBL_MAX), value_(0.0), default_(0.0)
{
}

DoubleRange::~DoubleRange()
{
}

bool DoubleRange::init(const std::string& s)
{
    min_   = -DBL_MAX;
    max_   =  DBL_MAX;
    value_ = 0.0;
    default_ = 0.0;
    format_ = AUTO;

    // Extract range from the parameter CSV string.
    // Parameters, p, for DoubleRange should be length 0, 1 or 2.
    //  - With 0 entries the range is unchanged (from -DBL_MAX to DBL_MAX)
    //  - With 1 entry the range is (p[0] to DBL_MAX)
    //  - With 2 entries the range is (p[0] to p[1])
    //
    // Notes: if p[0] is the string 'MIN' or p[1] is the string 'MAX'
    // these will resolve as -DBL_MAX and DBL_MAX respectively.
    bool ok = true;
    std::vector<std::string> p;
    p = oskar_settings_utility_string_get_type_params(s);
    if (p.size() == 0u) {
        return false;
    }
    else if (p.size() == 1u) {
        if (p[0] == "MIN") min_ = -DBL_MAX;
        else min_ = oskar_settings_utility_string_to_double(p[0], &ok);
    }
    else if (p.size() == 2u) {
        if (p[0] == "MIN")
            min_ = -DBL_MAX;
        else
            min_ = oskar_settings_utility_string_to_double(p[0], &ok);
        if (p[1] == "MAX")
            max_ = DBL_MAX;
        else
            max_ = oskar_settings_utility_string_to_double(p[1], &ok);
    }
    else {
        return false;
    }
    return ok;
}

bool DoubleRange::set_default(const std::string& s)
{
    format_ = (s.find_first_of('e') != std::string::npos) ? EXPONENT : AUTO;
    bool ok = from_string_(default_, s);
    if (ok) {
        value_ = default_;
    }
    return ok;
}

std::string DoubleRange::get_default() const
{
    return oskar_settings_utility_double_to_string_2(default_,
                                                     (format_==AUTO ? 'g' : 'e'));
}

bool DoubleRange::set_value(const std::string& s)
{
    format_ = (s.find_first_of('e') != std::string::npos) ? EXPONENT : AUTO;
    return from_string_(value_, s);
}

std::string DoubleRange::get_value() const
{
    return oskar_settings_utility_double_to_string_2(value_,
                                                     (format_ == AUTO ? 'g' : 'e'));
}

bool DoubleRange::is_default() const
{
    if (fabs(default_ - value_) < DBL_MIN) return true;
    else return false;
}

bool DoubleRange::from_string_(double& value, const std::string& s) const
{
    bool ok = true;
    double d = oskar_settings_utility_string_to_double(s, &ok);
    if (!ok) return false;
    if (d >= min_ && d <= max_) {
        value = d;
        return true;
    }
    else if (d < min_) {
        value = min_;
        return false;
    }
    else if (d > max_) {
        value = max_;
        return false;
    }
    return false;
}

bool DoubleRange::operator==(const DoubleRange& other) const
{
    return (fabs(value_ - other.value_) < DBL_MIN);
}

bool DoubleRange::operator>(const DoubleRange& other) const
{
    return value_ > other.value_;
}

} // namespace oskar

