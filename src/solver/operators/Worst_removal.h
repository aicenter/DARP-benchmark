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

#include <optional>
#include <vector>

#include "../DARP_context.h"
#include "../../Request.h"
#include "../../cost/Cost_evaluator.h"
#include "../../plan/VehiclePlan.h"

/**
 * @brief Worst removal operator: removes from a solution the requests whose removal saves the most cost. It is
 * independent of the solvers: a solver can use it to drop unprofitable requests permanently
 * (remove_while_improving), a metaheuristic as its destroy step (remove_worst).
 *
 * The removal candidates of a plan are the single requests and, optionally, the stops: maximal runs of at least two
 * consecutive actions of one type at one node (e.g., all packets picked up at one box in one visit). Removing a
 * candidate removes the pickups and drop-offs of all its requests. The saving of a candidate is the plan cost (the
 * weighted cost of Cost_evaluator) minus the cost of the plan without it. The plan without the candidate gets the
 * earliest schedule from the departure of the plan (the vehicle never departs earlier than before); a candidate
 * whose plan would break a max time, the max ride time or the max route duration is not considered.
 *
 * The operator follows the plan builders of the solvers in its memory use: init() sizes its buffers, which are then
 * reused by all evaluations and removals; a removal changes the plan in place (DARP_vehicle_plan::remove_actions)
 * and re-evaluates only the candidates of that plan.
 * The plans must not contain depot actions (the plans of the benchmark solutions do not).
 *
 * @tparam N node type
 * @tparam P plan type
 */
template<typename N, class P = VehiclePlan<N>>
class Worst_removal {
public:
	/**
	 * Removal candidate of a plan: \c length consecutive actions starting at \c first, each of a different request.
	 * A single request is the run of its pickup alone.
	 */
	struct Candidate {
		plan_size_type first;
		plan_size_type length;
		/** Plan cost minus the cost of the plan without the requests of the candidate. */
		cost_type saving;
	};

	explicit Worst_removal(const DARP_context<N>& context, bool stop_groups = true);

	/**
	 * Evaluates the plans and all their removal candidates. Must be called before the removals, and again when the
	 * plans changed otherwise than by this operator.
	 */
	void init(const std::vector<P>& plans);

	/**
	 * Removes candidates while it pays: with a zero \p threshold, while the removal decreases the average cost per
	 * served request (the candidate with the largest decrease first); with a positive \p threshold (e.g., the revenue
	 * of a request), while the saving per removed request exceeds it (the largest saving minus threshold first).
	 * @param plans the plans passed to init(), changed in place; emptied plans stay in the vector
	 * @param removed the removed requests are appended
	 * @return the number of removed requests
	 */
	unsigned remove_while_improving(
		std::vector<P>& plans, std::vector<const Request<N>*>& removed, cost_type threshold = 0);

	/**
	 * Removes \p request_count requests (or fewer, if there are not enough), the candidates with the largest saving
	 * per request first: the worst removal of the large neighborhood search (Ropke and Pisinger, 2006). Candidates
	 * with more requests than remain to be removed are skipped.
	 * @return the number of removed requests
	 */
	unsigned remove_worst(
		std::vector<P>& plans, unsigned request_count, std::vector<const Request<N>*>& removed);

	/** Total cost of the plans. */
	[[nodiscard]] cost_type get_cost() const {
		return cost;
	}

	/** Number of requests served by the plans. */
	[[nodiscard]] size_t get_served_request_count() const {
		return served_request_count;
	}

	/** Removal candidates of the plan \p plan_index. */
	[[nodiscard]] const std::vector<Candidate>& get_candidates(size_t plan_index) const {
		return candidates[plan_index];
	}

private:
	const Travel_time_provider<N>& travel_time_provider;
	const DARP_instance_configuration& configuration;
	// copy, as in the VGA plan search: the evaluations read its weights in the hot loop
	const Cost_evaluator evaluator;
	const bool stop_groups;

	std::vector<std::vector<Candidate>> candidates;
	std::vector<cost_type> plan_costs;
	cost_type cost{0};
	size_t served_request_count{0};

	// buffers of the plan being evaluated, indexed by the action positions
	std::vector<char> removed_flags;
	std::vector<plan_size_type> flagged_positions;
	std::vector<time_type> base_departures;
	std::vector<cost_type> base_prefix_costs;
	std::vector<time_type> candidate_arrivals;
	std::vector<time_type> candidate_departures;
	// depot of the plan being evaluated if the plan returns to it, otherwise nullptr
	const N* depot{nullptr};
	// plan times of the last simulation
	time_type simulated_departure{0};
	time_type simulated_end{0};

	void ensure_buffers(plan_size_type plan_length);

	void evaluate_plan(const std::vector<P>& plans, size_t plan_index);

	void evaluate_candidate(const P& plan, size_t plan_index, plan_size_type first, plan_size_type length);

	/** Flags the actions of the requests of a candidate, returns the first flagged position. */
	plan_size_type flag_candidate(const P& plan, plan_size_type first, plan_size_type length);

	void clear_flags();

	void prepare_plan(const P& plan);

	/**
	 * Simulates the plan without the flagged actions from the position \p from on, continuing the base simulation
	 * of the plan before it. The base simulation (\p base) writes the base buffers.
	 * @return the plan cost, or nothing if the plan breaks a constraint
	 */
	std::optional<cost_type> simulate(const P& plan, plan_size_type from, bool base);

	void apply(std::vector<P>& plans, size_t plan_index, const Candidate& candidate,
		std::vector<const Request<N>*>& removed);
};

#include "Worst_removal.tpp"
