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

#include <iostream>
#include "Reader.h"
#include "DARP_instance.h"
#include "Coordinate.h"
#include "travel_time_provider/Euclidean_travel_time_provider.h"
#include <string>

#include "Solution.h"


class Cordeau_node final : public Coordinate
{
public:
    Cordeau_node(float x, float y, unsigned int index);

    Cordeau_node(float x, float y);

    [[nodiscard]] double getX() const override;
    [[nodiscard]] double getY() const override;

	


private:
    const float x;
    const float y;
};

static_assert(std::is_move_constructible_v<Cordeau_node>);
static_assert(std::is_move_constructible_v<Action<Cordeau_node>>);
static_assert(std::is_move_constructible_v<Request<Cordeau_node>>);
static_assert(std::is_move_constructible_v<std::unique_ptr<Solution_iterator_interface<Cordeau_node>>>);
static_assert(!std::is_abstract_v<Solution<Cordeau_node>>);
static_assert(std::is_move_constructible_v<Solution<Cordeau_node>>);


class Cordeau_reader : public Reader<Cordeau_node> {
public:
    DARP_instance<Cordeau_node> read(std::filesystem::path filepath) override;
};


