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

#include <memory>

#include "Action.h"

template <typename N> class Request {
public:
	//static_assert(std::is_move_constructible<Request<N>>::value);
	
    Request(unsigned int index, unsigned int pickup_id, unsigned int dropoff_id, std::shared_ptr<N> pickup_node,
        unsigned int pickup_min_time, unsigned int pickup_max_time,
            std::shared_ptr<N> dropoff_node, unsigned int dropoff_min_time, unsigned int dropoff_max_time,
            unsigned short min_travel_time, unsigned short pickup_service_time = 0,
            unsigned short dropoff_service_time = 0);


	Request(const Request& other) = delete;
	Request(Request&& other) noexcept;
	Request& operator=(const Request & other) = delete;
	Request& operator=(Request && other) noexcept;

	/*
	 * returns a reference to the pickup action. Store it at your own risk, it becomes invalid on request move.
	 */
    const Service_action<N> &get_pickup() const;

    /*
     * returns a reference to the drop off action. Store it at your own risk, it becomes invalid on request move.
     */
    const Service_action<N> &get_dropoff() const;

    [[nodiscard]] unsigned short get_min_travel_time() const;

    [[nodiscard]] request_index_type get_index() const;

    void JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
    
    bool operator==(const Request<N> &other) const{
        return get_index() == other.get_index();
    }

    bool operator<(const Request<N> &other) const{
        return get_index() < other.get_index();
    }


    
private:
	/**
	 * ID of the request. Indexes for request must be between 0 and number of requests.
	 */
    const unsigned int index;
    Service_action<N> pickup;
    Service_action<N> drop_off;
    unsigned short min_travel_time;
};

#include "Request.tpp"


