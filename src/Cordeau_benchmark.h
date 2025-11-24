//
// Created by Fido on 2020-03-20.
//

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


