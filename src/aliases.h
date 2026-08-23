/*
 * MIT License
 *
 * Copyright (c) 2026 Czech Technical University in Prague
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. */
#pragma once
#include <cstdint>

using time_type = uint_least32_t;

/**
 * @brief Used in distance matrix. With 16 bits, the travel time is limited to approximately 18 hours.
*/
using travel_time_type = uint_least16_t;

using delay_type = uint_least16_t;

using request_index_type = uint_fast32_t;

/**
 * @brief Type for position in plan. Negative value (-1) indicates uninitialized value
 */
using index_in_plan = int_least16_t;

/**
 * @brief Type for plan size. With 16 bits, the plan size is limited to 65535.
 */
using plan_size_type = uint_fast16_t;

/**
 * @brief id of the delayed variants in chaining algorithm. ID is unique among variants for a single plan.
 */
using variant_id_type = uint_fast16_t;
