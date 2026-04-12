//
// Created by Fido on 2020-04-02.
//

#pragma once

#include <memory>
#include <optional>
#include <unordered_set>

#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

#include "aliases.h"
#include "solver/IH/IH_SVDARP_interfaces.h"


/**
 * @brief Abstract base class for all vehicle types.
 * Contains only the capacity which is common to all vehicles.
 */
class Vehicle_base {
public:
    explicit Vehicle_base(unsigned short capacity)
        : capacity(capacity) {}

    virtual ~Vehicle_base() = default;

    [[nodiscard]] unsigned short get_capacity() const {
        return capacity;
    }

    /**
     * @brief Serialize the vehicle to JSON. Default implementation does nothing.
     * Override in derived classes that need serialization.
     */
	virtual void JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const = 0;

protected:
    const unsigned short capacity;
};


/**
 * @brief Virtual vehicle class for scenarios where vehicles are spawned on demand.
 * Virtual vehicles do not have a fixed initial position - they start from where they are needed.
 * Instead of a position, they have a time_to_start property indicating when they become available.
 */
class Virtual_vehicle : public Vehicle_base {
public:
    /**
     * @brief Construct a virtual vehicle.
     * @param capacity The vehicle capacity.
     * @param time_to_start Time when the vehicle becomes available.
     * @param vehicle_count Number of vehicles of this type available.
     */
    Virtual_vehicle(
	    unsigned short capacity,
	    unsigned int time_to_start,
	    unsigned int vehicle_count = 1,
	    std::optional<std::unordered_set<request_index_type>>&& allowed_requests = std::nullopt
    );

    [[nodiscard]] unsigned int get_time_to_start() const;

    [[nodiscard]] unsigned int get_vehicle_count() const;

	[[nodiscard]] const std::optional<std::unordered_set<request_index_type>>& get_allowed_requests() const;

	void JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override;

private:
    unsigned int time_to_start;
    unsigned int vehicle_count;

	std::optional<std::unordered_set<request_index_type>> allowed_requests;
};


/**
 * @brief Template class for vehicles with a specific node type for initial position.
 * Inherits capacity from Vehicle_base.
 * @tparam N Node type for the initial position.
 */
template <typename N>
class Vehicle : public Vehicle_base {

public:
    Vehicle(unsigned int index, std::shared_ptr<N> initial_position, unsigned short capacity, time_type operation_start = 0);

    const N& get_init_position() const;

    [[nodiscard]] const std::shared_ptr<N>& get_init_position_ptr() const;

    void JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override;

    [[nodiscard]] unsigned int get_index() const;

    [[nodiscard]] time_type get_operation_start() const;

private:
    unsigned int index;
    const std::shared_ptr<N> init_position;
	time_type operation_start;
};

#include "Vehicle.tpp"


