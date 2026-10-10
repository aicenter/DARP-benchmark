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
#include <vector>
#include <optional>
#include <string>
#include <filesystem>
#include <memory>
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include <boost/iterator/iterator_adaptor.hpp>

#include "DARP_instance.h"
#include "cost/Cost_evaluator.h"
#include "plan/VehiclePlan.h"

/**
 * Copyable wrapper for solution iterator interface
 */
template<typename N>
class Solution_iterator_wrapper;

/*
 * Solution iterator interface. There has to be an interface to erase the plan type, as the solution interface is
 * agnostic to a concrete type.
 */
template <typename N>
class Solution_iterator_interface{

public:
	virtual ~Solution_iterator_interface() = default;

    virtual const Benchmark_vehicle_plan& operator* () const = 0;
    
    virtual const Solution_iterator_interface& operator++ () = 0;

    virtual bool operator!= (const Solution_iterator_interface& other) const = 0;

    virtual Solution_iterator_wrapper<N> get_copyable_wrapper() = 0;

protected:
	Solution_iterator_interface() = default;
	Solution_iterator_interface(const Solution_iterator_interface& other) = default;
	Solution_iterator_interface(Solution_iterator_interface&& other) noexcept = default;
	Solution_iterator_interface& operator=(const Solution_iterator_interface& other) = default;
	Solution_iterator_interface& operator=(Solution_iterator_interface&& other) noexcept = default;
};


/*
 * Iterator adapter. It transforms the vector iterator over concrete plan type into a reference to the
 * Benchmark_vehicle_plan class
 */
template<typename N, Benchmark_plan P>
class Solution_iterator_adapter : public boost::iterator_adaptor<
    Solution_iterator_adapter<N,P>,
    typename std::vector<P>::const_iterator,
	boost::use_default,
	boost::forward_traversal_tag,
	const Benchmark_vehicle_plan&
>, public Solution_iterator_interface<N>
{
    friend class boost::iterator_core_access;

    using AdapterType = boost::iterator_adaptor<
        Solution_iterator_adapter<N,P>,
        typename std::vector<P>::const_iterator,
		boost::use_default,
		boost::forward_traversal_tag,
		const Benchmark_vehicle_plan&
    >;

public:
    using AdapterType::iterator_adaptor;

    /*
     * The following functions just transfers the calls to the Solution iterator interface to the adapter
     */
	const Benchmark_vehicle_plan& operator*() const override{
        return AdapterType::operator*();
	}
	const Solution_iterator_interface<N>& operator++() override {
        return AdapterType::operator++();
	}
	bool operator!=(const Solution_iterator_interface<N>& other) const override {
        return boost::iterators::operator!=(*this, dynamic_cast<const Solution_iterator_adapter<N, P>&>(other));
	}

	/**
	 * @brief Creates a wrapper of the adapter that can be copied (The polymorphic iterator interface class cannot
	 * be copied). 
	 * @return Aa wrapper of the adapter.
	*/
	Solution_iterator_wrapper<N> get_copyable_wrapper() override;

private:
    /**
     * Here is the magic, we are hiding the dereference function from iterator_adapter
     */
    [[nodiscard]] const Benchmark_vehicle_plan& dereference() const;
};



/**
 * @brief Copyable wrapper for Solution_iterator_interface
 * @tparam N node type
*/
template<typename N>
class Solution_iterator_wrapper {
public:
	explicit Solution_iterator_wrapper(Solution_iterator_interface<N>* iterator)
		: iterator(iterator) {
	}

	const Benchmark_vehicle_plan& operator*() const {
        return iterator->operator*();
	}
	const Solution_iterator_interface<N>& operator++() {
        return iterator->operator++();
	}
	bool operator!=(const Solution_iterator_wrapper<N>& other) const {
        return iterator->operator!=(*other.iterator);
	}

private:
    Solution_iterator_interface<N>* iterator;
};


/**
 * @brief Solution interface for node agnostic solvers. It is used to solve the DARP benchmark instances
 * without knowing the specific node type used.
 * @tparam N node type
*/
class Solution_interface_node_agnostic {
public:
	virtual ~Solution_interface_node_agnostic() = default;

	[[nodiscard]] virtual rapidjson::StringBuffer JSON_serialize(unsigned short resolution) const;
};


/**
 * @brief Solution interface. It exposes the operations needed outside the solvers, e.g. serialization.
 * @tparam N node type.
*/
template<typename N>
class Solution_interface
	// : public Solution_interface_node_agnostic
{
public:
	~Solution_interface() = default;
	[[nodiscard]] rapidjson::StringBuffer JSON_serialize(unsigned short resolution = 1) const;


	Solution_interface(
		const cost_type cost,
		std::vector<const Request<N>*>&& dropped_requests,
		bool feasible
	);

	/**
	 * @brief Default constructor. It is used when the solution is not found.
	 */
	Solution_interface(): feasible(false) {};


	[[nodiscard]] unsigned int get_dropped_request_count() const;

	[[nodiscard]] virtual unsigned long get_total_passenger_delay() const { return 0; }

	[[nodiscard]] virtual unsigned long get_total_ride_time() const { return 0; }

	[[nodiscard]] virtual unsigned long get_total_driving_time() const { return 0; }

	[[nodiscard]] virtual unsigned long get_plan_count() const { return 0; }

    [[nodiscard]] const std::vector<const Request<N>*> &get_dropped_requests() const {
        return this->dropped_requests;
    }

	[[nodiscard]] bool is_feasible() const {
		return this->feasible;
	}

	[[nodiscard]] virtual std::string export_simple_csv() const;

protected:
	cost_type cost;
    std::vector<const Request<N>*> dropped_requests;
	bool feasible{true};

	Solution_interface(const Solution_interface& other) = default;
	Solution_interface(Solution_interface&& other) noexcept = default;
	Solution_interface& operator=(const Solution_interface& other) = default;
	Solution_interface& operator=(Solution_interface&& other) noexcept = default;

    virtual bool check();
	virtual void JSON_serialize_extra_fields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;
private:
	[[nodiscard]] virtual std::unique_ptr<Solution_iterator_interface<N>> begin() const = 0;
    [[nodiscard]] virtual std::unique_ptr<Solution_iterator_interface<N>> end() const = 0;
};


/**
 * @brief Class for storing the DARP solution. It serves both for storing temporary solutions inside solvers, and
 * to return the solution to the benchmark.
 * @tparam N node type.
 * @tparam P plan type
*/
template <typename N, Benchmark_plan P = VehiclePlan<N>>
class Solution: public Solution_interface<N>
{
public:
	/**
	 * @brief Constructor for the solution found by the solver. This one is used if we have already a collection of
	 * dropped requests (even empty).
	 * @param vehicle_plans
	 * @param cost
	 * @param dropped_requests
	 */
    Solution(
        std::vector<P>&& vehicle_plans, 
        cost_type cost, 
        std::vector<const Request<N>*>&& dropped_requests
    );

	/**
	 * @brief Constructor for the solution found by the solver. This one is used when we do not record the dropped
	 * requests.
	 * @param instance
	 * @param vehicle_plans
	 */
    Solution(
        const DARP_instance<N>& instance, 
        std::vector<P>&& vehicle_plans
    );

	Solution(
		const DARP_instance<N>& instance,
		std::vector<P>&& vehicle_plans,
		std::unique_ptr<std::vector<Vehicle<N>>>&& fleet_sizing_vehicle_backing_par
	);

	/**
	 * Constructor for potentially infeasible solution. This one is used when the solution may be infeasible, but
	 * there is some content (in contrast to the default constructor which is for failed solution search).
	 * @param vehicle_plans
	 * @param cost
	 * @param dropped_requests
	 * @param is_feasible
	 */
    Solution(
        std::vector<P>&& vehicle_plans,
        cost_type cost,
        std::vector<const Request<N>*>&& dropped_requests,
        bool is_feasible
    );

	/**
	 * Deserialization from JSON when plans may reference a shared \c Virtual_vehicle (not in DARP_instance).
	 * Backing storage must outlive plans; it is kept in \c virtual_vehicle_backing (declared before plans).
	 */
	Solution(
		std::vector<P>&& vehicle_plans,
		cost_type cost,
		std::vector<const Request<N>*>&& dropped_requests,
		std::optional<Virtual_vehicle>&& virtual_vehicle_backing_par
	);

	Solution(
		std::vector<P>&& vehicle_plans,
		cost_type cost,
		std::vector<const Request<N>*>&& dropped_requests,
		std::optional<Virtual_vehicle>&& virtual_vehicle_backing_par,
		std::optional<std::vector<Vehicle<N>>>&& fleet_sizing_vehicle_backing_par
	);

	/**
	 * @brief Default constructor. It is used when the solution is not found.
	 */
	Solution(): Solution_interface<N>() {};

	Solution(const Solution&);
	Solution& operator=(const Solution&);
	Solution(Solution&&) noexcept = default;
	Solution& operator=(Solution&&) noexcept = default;

    [[nodiscard]] const std::vector<P>& get_plans() const;

	/**
	 * @brief Sets the cost of every plan and the solution cost to the solver-independent evaluation of the plans
	 * with the cost weights of the instance (see Cost_evaluator).
	 */
	void evaluate_costs(const DARP_instance<N>& instance);

    [[nodiscard]] cost_type get_cost() const;

	[[nodiscard]] unsigned long get_total_passenger_delay() const override;

	[[nodiscard]] unsigned long get_total_ride_time() const override;

	[[nodiscard]] unsigned long get_total_driving_time() const override;

	[[nodiscard]] unsigned long get_plan_count() const override;

    unsigned int get_non_empty_plan_count();

    const P& operator[] (int index) const;

	/**
	 * @brief \c true when this solution uses fleet-sizing materialized vehicle backing (\c deserialize_json with
	 * \c problem_type::fleet_sizing). \c false for classic DARP. When \c true, \c fleet_sizing_vehicle_backing may still
	 * hold an empty vector (e.g. no materialized vehicles in the JSON).
	 */
	[[nodiscard]] bool is_fleet_sizing_vehicle_backing_engaged() const noexcept;

protected:
	std::optional<problem_type> problem{};
	/**
	 * When \c std::nullopt, this solution is not in fleet-sizing mode (no fleet JSON materialization backing).
	 * When engaged, holds \c Vehicle<N> materialized from solution JSON for fleet-sizing (vector may be empty, e.g. no
	 * materialized vehicles needed). Declared before plans.
	 */
	std::optional<std::vector<Vehicle<N>>> fleet_sizing_vehicle_backing{};
	/**
	 * Owns the single \c Virtual_vehicle referenced by plans after JSON deserialization (when present; all virtual
	 * plans share this object). Declared before plans so plans are destroyed first.
	 */
	std::optional<Virtual_vehicle> virtual_vehicle_backing{};
	std::vector<P> plans;

	/**
	 * @brief Checks the solution integrity. This method is automatically called in all constructors when using
	 * the debug mode. 
	 * @return Always returns true, the problems are propagated using asserts.
	*/
	bool check();
	void JSON_serialize_extra_fields(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override;

private:
    [[nodiscard]] std::unique_ptr<Solution_iterator_interface<N>> begin() const override;
    [[nodiscard]] std::unique_ptr<Solution_iterator_interface<N>> end() const override;

	int non_empty_plan_count{-1};
};

template <typename N, Benchmark_plan P = VehiclePlan<N>>
Solution<N,P> deserialize_json(std::filesystem::path path, const DARP_instance<N>& darp_instance);

#include "Solution.tpp"
