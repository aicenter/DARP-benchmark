#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "IH_SVDARP_interfaces.h"
#include "../DARP_context.h"
#include "IH_vehicle_plan_builder.h"


/**
 * Solver for the Single Vehicle Dial-a-Ride Problem (SVDARP) using the Insertion Heuristic (IH) method.
 * This solver use the earliest insertion strategy as the extra time after minimal time is considered as a delay.
 * Note that this is in contrast to the famous 8-step evaluation procedure ("A tabu search heuristic for the
 * static multi-vehicle dial-a-ride problem" (2003), p 9 (587))., where the ride time is minimized by delaying the
 * actions maximally without breaking the constraints.
 * @tparam N node type
 * @tparam V vehicle type
 * @tparam A action data type
 * @tparam P plan builder type
 */
template<typename N, IH_SVDARP_vehicle<N> V, IH_SVDARP_action<N> A, Vehicle_plan_builder_plan<V,A> P>
class SVDARP {
	struct Temporal_action_bounds {
		std::vector<std::int64_t> earliest_service_starts;
		std::vector<std::int64_t> latest_service_starts;
	};

public:
	struct Temporal_insertion_position_range {
		index_in_plan first{0};
		index_in_plan last{0};

		[[nodiscard]] bool empty() const {
			return first > last;
		}
	};

	static Temporal_insertion_position_range compute_temporal_insertion_position_range(
		const std::vector<std::int64_t>& earliest_service_starts,
		const std::vector<std::int64_t>& latest_service_starts,
		time_type min_time,
		time_type max_time
	) {
		if(earliest_service_starts.empty()) {
			return {0, 0};
		}

		const auto first_possible_position = static_cast<index_in_plan>(
			std::lower_bound(latest_service_starts.begin(), latest_service_starts.end(), min_time)
			- latest_service_starts.begin()
		);
		const auto last_possible_position = static_cast<index_in_plan>(
			std::upper_bound(earliest_service_starts.begin(), earliest_service_starts.end(), max_time)
			- earliest_service_starts.begin()
		);

		return {first_possible_position, last_possible_position};
	}

	explicit SVDARP(const DARP_context<N>& context_par): context(context_par) {
	}

	bool adjust_times(
		unsigned short action_position,
        IH_vehicle_plan_builder<V, A, P>& vehicle_plan,
        Adjustment_reason initial_reason
       ) const {

    	// key is the index in the plan
    	short stack_top_index = 0;
    	std::array<Adjust_times_data, Vehicle_plan_builder<V, A, P>::adj_stack_size>& adj_stack = vehicle_plan.get_adj_stack();
    	adj_stack[stack_top_index] = {action_position, initial_reason};
    	

    	// time adjustments init
        const auto adjustment_times_start_index = vehicle_plan.compute_time_adjustment_start_index();
    	std::vector<int>& time_adjustments = vehicle_plan.get_time_adjustments();
    	time_adjustments[adjustment_times_start_index] = 1; // change indicator

    	
        while (stack_top_index >= 0) {
            const Adjust_times_data& to_resolve = adj_stack[stack_top_index];
        	const Adjustment_reason reason = to_resolve.adjustment_reason;
        	const unsigned short drop_off_position_in_plan = to_resolve.index;
            --stack_top_index;
            A& drop_off_action_data = vehicle_plan[drop_off_position_in_plan];
            assert(drop_off_action_data.get_action_type() == Action_type::dropoff);

            unsigned int drop_off_service_start_time
                = std::max(drop_off_action_data.get_arrival_time(), drop_off_action_data.get_min_time());

        	// drop off departure time initialization
        	if(drop_off_action_data.get_departure_time() < 0) {
        		drop_off_action_data.set_departure_time(
                    drop_off_service_start_time + drop_off_action_data.get_service_duration());
        	}

            // difference between current ride time (which breaks constraint) and max ride/route time
            int diff;
            A* pickup_action_data;
            unsigned int travel_time_to_depot = 0;
            unsigned short pickup_action_data_index;
        	
        	unsigned short pickup_position_in_plan;
            if (reason == Adjustment_reason::max_ride_time) {
            	pickup_action_data_index = drop_off_action_data.get_other_action_data_index();
            	
                pickup_action_data = &vehicle_plan.get_action_data()[pickup_action_data_index];
            	pickup_position_in_plan = pickup_action_data->get_position_in_plan();
            	assert(vehicle_plan.get_action_order()[pickup_position_in_plan] == pickup_action_data_index);
            	
                diff = drop_off_service_start_time - pickup_action_data->get_departure_time()
            		- this->context.darp_instance_configuration()->get_max_ride_time();
            }
            else {
            	pickup_action_data_index = vehicle_plan.get_action_order()[0];
            	
                pickup_action_data = &vehicle_plan.get_action_data()[pickup_action_data_index];
            	pickup_position_in_plan = pickup_action_data->get_position_in_plan();
            	assert(vehicle_plan.get_action_order()[pickup_position_in_plan] == pickup_action_data_index);
            	
                travel_time_to_depot = this->context.travel_time_provider()->get_travel_time(
	                drop_off_action_data.get_node(), vehicle_plan.get_vehicle().get_init_position());
                diff = drop_off_service_start_time + drop_off_action_data.get_service_duration()
                    + travel_time_to_depot - this->context.darp_instance_configuration()->get_max_route_duration() - vehicle_plan.get_departure_time();
            }
        	assert(pickup_action_data->get_action_type() == Action_type::pickup);
        	
            const std::vector<short>& action_order = vehicle_plan.get_action_order();
        	const auto pickup_time_adjustment_index = adjustment_times_start_index + 2 + action_order[pickup_position_in_plan] * 2;

            // it can happen that the problem was already solved by moving different action
            if (diff <= 0) {
                continue;
            }

			// fail if pick up cannot be delayed
			if (pickup_action_data->get_departure_time() + diff - pickup_action_data->get_service_duration()
				> (int) pickup_action_data->get_max_time()) {
				return false;
			}

            if (reason == Adjustment_reason::max_ride_time) {
                // set new pickup departure time
                pickup_action_data->set_departure_time(pickup_action_data->get_departure_time() + diff);
            	time_adjustments[pickup_time_adjustment_index + 1] += diff;
            }
            else {
                // check that we can postpone the arrival to the first action
                if (pickup_action_data->get_arrival_time() + diff > pickup_action_data->get_max_time()) {
                    return false;
                }

                // set new pickup arrival time
                pickup_action_data->set_arrival_time(pickup_action_data->get_arrival_time() + diff);

            	// we do not have to save the adjustment for the current action
            	if(pickup_position_in_plan < vehicle_plan.get_action_data_used_length() - 1){
            		time_adjustments[pickup_time_adjustment_index] += diff;
                }

            	// new vehicle departure time
                vehicle_plan.set_departure_time(vehicle_plan.get_departure_time() + diff);
            	time_adjustments[adjustment_times_start_index + 1] += diff;

            	// set new pickup departure time 
                unsigned int old_departure_time = pickup_action_data->get_departure_time();
                pickup_action_data->set_departure_time(std::max(old_departure_time, pickup_action_data->get_arrival_time()
                    + pickup_action_data->get_service_duration()));
                diff = pickup_action_data->get_departure_time() - old_departure_time; // diff recomputation

            	// we do not have to save the adjustment for the current action
            	if(pickup_position_in_plan < vehicle_plan.get_action_data_used_length() - 1){
            		time_adjustments[pickup_time_adjustment_index + 1] += diff;
                }
            }

        	const auto current_action_data_index = vehicle_plan.get_active_length() - 1;

        	
        	// here, we delay all actions after the current action
        	for (unsigned short action_index = pickup_position_in_plan + 1; ; ++action_index) {
                A& action_data = vehicle_plan[action_index];

            	// arrival time adjustment
                unsigned int new_arrival_time = action_data.get_arrival_time() + diff;
                // check max arrival time
                if (new_arrival_time > action_data.get_max_time()) {
                    return false;
                }
                action_data.set_arrival_time(new_arrival_time);
        		
                plan_size_type action_time_adjustment_index{0};
                // we do not have to save the adjustment for the current action
        		const unsigned short action_data_index = action_order[action_index];
                if(action_data_index < current_action_data_index){
                	action_time_adjustment_index = adjustment_times_start_index + 2 + action_order[action_index] * 2;
                	time_adjustments[action_time_adjustment_index] += diff;
                }

            	// departure time adjustment
                const unsigned int current_departure_time = action_data.get_departure_time();
            	const unsigned int new_departure_time = std::max(current_departure_time,
                        action_data.get_arrival_time() + action_data.get_service_duration());

            	// delay update
                diff = new_departure_time - current_departure_time;

            	//if this is the drop off corresponding to the just adjusted pickup, we end here
            	if(action_index == drop_off_position_in_plan) {

            		// if diff is still > 0, max ride time cannot be fixed
        			if(diff > 0) {
        				return false;
        			}

            		// else we can stop, we cannot cause max ride/route time exceeding on the request being adjusted
	                break; 
            	}
            	
                action_data.set_departure_time(new_departure_time);
                
                // we do not have to save the adjustment for the current action
                if(action_data_index < current_action_data_index){
                	time_adjustments[action_time_adjustment_index + 1] += diff;
                }

            	// possible reinsertion to the queue
                if (action_data.get_action_type() == Action_type::dropoff) {
                    unsigned int service_time
                        = std::max(action_data.get_arrival_time(), action_data.get_min_time());

                    // max ride time check
                    if (service_time - vehicle_plan.get_other(action_data).get_departure_time() 
						> this->context.darp_instance_configuration()->get_max_ride_time()) {
						++stack_top_index;
                        assert(stack_top_index < (Vehicle_plan_builder<V, A, P>::adj_stack_size));
                        adj_stack[stack_top_index] = {action_index, Adjustment_reason::max_ride_time};
                    }

					// max route time check - max route time cannot be exceeded by time adjustment
                }

                // if the delay is 0, we can stop
                if (diff <= 0) {
                    break;
                }
            }
        }
    	
		return true;
    }

	
	bool insert_into_plan(
        IH_vehicle_plan_builder<V, A, P>& plan,
	    index_in_plan action_index_in_plan, 
	    const bool pickup
    ) const {
    	
    	// index of the action data in the action data vector (not the action order)
    	const index_in_plan new_action_data_index
			= pickup
				? static_cast<index_in_plan>(plan.get_action_data_used_length() - 2)
				: static_cast<index_in_plan>(plan.get_action_data_used_length() - 1);
    	A& new_action_data = plan.get_action_data()[new_action_data_index];
    	const V& vehicle = plan.get_vehicle();

    	/*
    	 * Processing way to new action
    	 */
    	unsigned current_time; // time in the new plan in seconds
    	unsigned travel_time_to;
        const bool first_action = action_index_in_plan == 0;
    	A* before_action_data = nullptr;

    	if(first_action) {
    		current_time = plan.get_operating_start();
    		travel_time_to = this->context.travel_time_provider()->get_travel_time(
	            vehicle.get_init_position(), new_action_data.get_node());
    	}
        else {
	        // action data before index in plan
    		before_action_data = &plan[action_index_in_plan - 1];
        	current_time = before_action_data->get_departure_time();
    		travel_time_to = this->context.travel_time_provider()->get_travel_time(
	            before_action_data->get_node(), new_action_data.get_node());
        }

    	current_time += travel_time_to;

    	if(current_time > new_action_data.get_max_time()) {
    		return false;
    	}

		// setting the arrival and departure time for the new action
        auto min_service_time = std::max(new_action_data.get_min_time(), current_time);
        //if(first_action) {
	       // new_action_data.set_arrival_time(min_service_time); // do not arrive to early for the first action
        //}
        //else{
    		new_action_data.set_arrival_time(current_time);
    	//}
    	current_time = min_service_time;
    	current_time += new_action_data.get_service_duration();
    	new_action_data.set_departure_time(current_time);
		
    	// Cost backup
    	if(pickup) {
    		plan.set_cost_before_pickup();
    	}
        else {
	        plan.set_cost_before_drop_off();
        }

    	/*
    	 *Processing way from new action
    	 */
    	bool last_action = action_index_in_plan == new_action_data_index;
    	    	
    	//// last action, do not return to depot
    	//if(last_action && !this->return_to_depot) {
    	//	plan.set_cost(plan.get_cost() + travel_time_to);
    	//	plan.get_action_order()[plan.get_length() - 1] = new_action_data_index;
    	//	return true;
    	//}

    	unsigned int direct_travel_time;
    	unsigned int travel_time_from;
    	int diff;  // the delay off the actions following the currently inserted action
    	
    	// last action, return to depot
    	if(last_action) {

    		// travel time from
    		if(this->context.darp_instance_configuration()->is_return_to_depot()) {
    			travel_time_from = this->context.travel_time_provider()->get_travel_time(
	                new_action_data.get_node(), vehicle.get_init_position());
    		}
            else {
	            travel_time_from = 0;
            }

    		// diff
    		//diff = current_time + travel_time_from - plan.get_arrival_time();
    		diff = 0;

    		//direct travel time
    		if(first_action || !this->context.darp_instance_configuration()->is_return_to_depot()) {
    			direct_travel_time = 0;
    		}
            else {
	            direct_travel_time = this->context.travel_time_provider()->get_travel_time(
		            before_action_data->get_node(), vehicle.get_init_position());
            }
    	}
        else {
	        // action data of the action right after the newly added one
    		A& after_action_data = plan[action_index_in_plan];

        	// travel time from the new action to the first action after it
        	travel_time_from = this->context.travel_time_provider()->get_travel_time(
	            new_action_data.get_node(), after_action_data.get_node());

        	// diff
			const auto old_arrival_time = after_action_data.get_arrival_time();
			const auto new_arrival_time = current_time + travel_time_from;
			// we have to check that new arrival time is later than the old one. This is only necessary for plans that
			// does not have all times minimized (e.g. plans resulting from evaluate function from HALNS).
        	diff = new_arrival_time - old_arrival_time;

        	// direct travel time
        	if(first_action) {
        		direct_travel_time = after_action_data.get_arrival_time() - plan.get_departure_time();
        	}
            else {
	            direct_travel_time = after_action_data.get_arrival_time() - before_action_data->get_departure_time();
            }
        }

    	const auto adjustment_times_start_index = plan.compute_time_adjustment_start_index();
    	std::vector<int>& time_adjustments = plan.get_time_adjustments();
    	
    	// departure time needs to be zeroed if we add action to the beginning of the plan
    	if(new_action_data_index == 0 && plan.get_departure_time() > plan.get_operating_start()) {
    		const unsigned int old_departure_time = plan.get_departure_time();
    		plan.set_departure_time(plan.get_operating_start());
    		time_adjustments[adjustment_times_start_index] = 1; // change indicator
    		time_adjustments[adjustment_times_start_index + 1] = -((int) old_departure_time);
    	}

    	// action reordering
    	// TODO this can be deferred to the end of the function. The adjust times method have to be modified first, however.
        std::vector<short>& action_order = plan.get_action_order();
        for(index_in_plan i = new_action_data_index; i > action_index_in_plan; --i) {     	
			action_order[i] = action_order[i-1];
			plan[i].set_position_in_plan(i);
        }
    	action_order[action_index_in_plan] = new_action_data_index;
    	new_action_data.set_position_in_plan(action_index_in_plan);

    	//for each action after currently inserted one
	    for(index_in_plan action_index = action_index_in_plan; action_index <= new_action_data_index; ++action_index){

	        A& action_data = plan[action_index];

	    	// delaying action after currently inserted one
	    	if(action_index > action_index_in_plan){
	    		assert(action_data.get_arrival_time() >= 0);
	    		
                // check max time check for the action
		        if(action_data.get_max_time() < action_data.get_arrival_time() + diff){
	        		plan.remove_lastly_added_action(pickup);
		            return false;
		        }
	    		
	    		action_data.set_arrival_time(action_data.get_arrival_time() + diff);
		        const unsigned short action_data_index = plan.get_action_order()[action_index];
		        const auto time_adjustment_index = adjustment_times_start_index + 2 + 2 * action_data_index;
	    		time_adjustments[adjustment_times_start_index] = 1; // change indicator TODO only change it in first iteration
	    		time_adjustments[time_adjustment_index] += diff;
	    		const unsigned int min_service_end_time = action_data.get_min_service_end();
	    		const unsigned int departure_time = action_data.get_departure_time();
	    		if(min_service_end_time > departure_time) {
	    			assert(static_cast<int>(min_service_end_time - departure_time) <= diff);
	    			diff = min_service_end_time - departure_time;
	    			action_data.set_departure_time(min_service_end_time);
	    			time_adjustments[time_adjustment_index + 1] += diff;
	    		}
                else {
	                diff = 0;
                }
	            //else {
	            //    break;
	            //}
            }

	    	// max ride time check
			if(action_data.get_action_type() == Action_type::dropoff){
	            if(this->context.darp_instance_configuration()->get_max_ride_time() && action_data.get_service_start_time() 
                    - plan.get_other(action_data).get_departure_time() > this->context.darp_instance_configuration()->get_max_ride_time()
                ){
	                if(!adjust_times(action_index, plan, Adjustment_reason::max_ride_time)) {
	                	plan.remove_lastly_added_action(pickup);
	                    return false;
	                }
	            }

				// check max route time
				// TODO the performance can be probably slightly improved by checking max route time for pickup too.
				// However, it requires some changes in adjust times method.
		        if(this->context.darp_instance_configuration()->get_max_route_duration()){
		        	unsigned long plan_arrival_time;
		        	if(this->context.darp_instance_configuration()->is_return_to_depot()){
		        		// add cost of returning to depot
						const unsigned int travel_time_to_depot = this->context.travel_time_provider()->get_travel_time(
							action_data.get_node(), vehicle.get_init_position());
		        		plan_arrival_time = action_data.get_departure_time() + travel_time_to_depot;
                    }
                    else{
                    	plan_arrival_time = action_data.get_arrival_time();
                    }
	                if(plan_arrival_time - plan.get_departure_time() > this->context.darp_instance_configuration()->get_max_route_duration()){
			            if (!adjust_times(action_index, plan, Adjustment_reason::max_route_time)) {
	            			plan.remove_lastly_added_action(pickup);
			                return false;
			            }
                    }
		        }
	        }

	    	if(diff == 0) {
	    		break;
	    	}
	    }

		// cost update
    	const unsigned int cost_increment = travel_time_to + travel_time_from - direct_travel_time;
	    plan.set_cost(plan.get_cost() + cost_increment);

	    return true;
	}

	/**
	 * @brief Inserts request into fixed plan optimally while keeping the order of the actions already present
	 * in the plan. The \p plan is modified in place in case and only if there is a feasible plan found with the cost
	 * increment smaller than the \p min increment.
	 *
	 * Note that this method does not finalize the plan builder! This is because many calls of this method are
	 * expected so it is better to leave the finalization to the caller that can perform it only once when the plan
	 * is complete. Notably, this applies to the plan departure and arrival time.
	 * @param pickup_action_data 
	 * @param drop_off_action_data 
	 * @param plan the plan builder to be modified
	 * @param min_increment cutoof for the cost increment of the plan. If the cost increment of the plan is greater
	 * than \p min_increment, the plan is not modified.
	 * @return the minimum increment. If no improving plan was found, than the return value equals \p min_increment.
	*/
	unsigned int insert_request_into_plan_optimally(
        A& pickup_action_data,
        A& drop_off_action_data,
        IH_vehicle_plan_builder<V, A, P>& plan,
        unsigned long min_increment,
		plan_size_type temporal_pruning_min_plan_length = 32
    ) const {
        plan.add_new_request_data(pickup_action_data, drop_off_action_data);
    	
		const auto existing_action_count = static_cast<index_in_plan>(plan.get_action_data_used_length() - 2);
		Temporal_insertion_position_range pickup_range{0, existing_action_count};
		Temporal_insertion_position_range drop_off_range{0, existing_action_count};

		const bool temporal_pruning_enabled =
			temporal_pruning_min_plan_length > 0
			&& existing_action_count >= static_cast<index_in_plan>(temporal_pruning_min_plan_length);

		if(temporal_pruning_enabled && existing_action_count > 0) {
			const Temporal_action_bounds action_bounds = compute_temporal_action_bounds(plan, existing_action_count);
			pickup_range = compute_temporal_insertion_position_range(
				action_bounds.earliest_service_starts,
				action_bounds.latest_service_starts,
				pickup_action_data.get_min_time(),
				pickup_action_data.get_max_time()
			);
			drop_off_range = compute_temporal_insertion_position_range(
				action_bounds.earliest_service_starts,
				action_bounds.latest_service_starts,
				drop_off_action_data.get_min_time(),
				drop_off_action_data.get_max_time()
			);
		}

	    unsigned short free_capacity = plan.get_vehicle().get_capacity();
    	unsigned long old_cost = plan.get_cost();
    	IH_vehicle_plan_builder<V, A, P> best_plan = plan;

	    for(
			index_in_plan pickup_option_index = 0;
			pickup_option_index < static_cast<index_in_plan>(plan.get_action_data_used_length() - 1);
			pickup_option_index++
		){

	        // continue if the vehicle is full
	        if(free_capacity > 0 && !pickup_range.empty()
				&& pickup_option_index >= pickup_range.first && pickup_option_index <= pickup_range.last){

                // insert pickup
	        	bool success = insert_into_plan(plan, pickup_option_index, true);
                
	        	if(success) {

		            unsigned long cost_increment = plan.get_cost() - old_cost;
                    
	        		if(cost_increment > min_increment) {
	        			plan.remove_lastly_added_action(true, true);
	        		}
                    else {
                        unsigned free_capacity_drop_off = free_capacity - 1; // now we count the succesfull pickup

						if(!drop_off_range.empty()) {
							const auto first_drop_off_option_index = std::max<index_in_plan>(
								pickup_option_index + 1,
								drop_off_range.first + 1
							);
							const auto last_drop_off_option_index = std::min<index_in_plan>(
								static_cast<index_in_plan>(plan.get_action_data_used_length() - 1),
								drop_off_range.last + 1
							);
							bool capacity_allows_later_drop_off = first_drop_off_option_index <= last_drop_off_option_index;

							for(
								index_in_plan drop_off_option_index = pickup_option_index + 1;
								capacity_allows_later_drop_off && drop_off_option_index < first_drop_off_option_index;
								drop_off_option_index++
							) {
								if(drop_off_option_index < static_cast<index_in_plan>(plan.get_active_length() - 1)){
									if(plan[drop_off_option_index].get_action_type() == Action_type::pickup){
										if(free_capacity_drop_off == 0) {
											capacity_allows_later_drop_off = false;
										}
										else {
											--free_capacity_drop_off;
										}
									}
									else{
										++free_capacity_drop_off;
									}
								}
							}

							for(
								index_in_plan drop_off_option_index = first_drop_off_option_index;
								capacity_allows_later_drop_off && drop_off_option_index <= last_drop_off_option_index;
								drop_off_option_index++
							) {
								// insert drop off
								success = insert_into_plan(plan, drop_off_option_index, false);
								if(success){
									cost_increment = plan.get_cost() - old_cost;
									if(cost_increment < min_increment) {
										min_increment = cost_increment;
										best_plan = plan;
									}
									plan.remove_lastly_added_action(false, true);
								}

								// check the capacity and change free capacity for next index
								if(drop_off_option_index < static_cast<index_in_plan>(plan.get_active_length() - 1)){
									if(plan[drop_off_option_index].get_action_type() == Action_type::pickup){
										if(free_capacity_drop_off == 0) {
											break;
										}
										--free_capacity_drop_off;
									}
									else{
										++free_capacity_drop_off;
									}
								}
							}
						}
	        			plan.remove_lastly_added_action(true, true);
                    }
	        	}
	        }

	        // change free capacity for next index
            if(pickup_option_index < static_cast<index_in_plan>(plan.get_active_length() - 1)){
	            if(plan[pickup_option_index].get_action_type() == Action_type::pickup){
	                --free_capacity;
	            }
	            else{
	                ++free_capacity;
	            }
            }
	    }

    	plan = best_plan;
    	return min_increment;
	}

	void finalize_plan(IH_vehicle_plan_builder<V, A, P>& plan) const {
		// adjust arrival time of the first action so that the vehicle does not wait at the pickup location
		auto& first_action_data = plan[0];
		auto travel_time_to_start = this->context.travel_time_provider()->get_travel_time(
			plan.get_vehicle().get_init_position(),
			first_action_data.get_action().get_node()
		);
		auto start_time = plan.get_operating_start();
		if (start_time + travel_time_to_start < first_action_data.get_min_time()) {
			first_action_data.set_arrival_time(first_action_data.get_min_time());
		}

		// set plan departure time to so that the vehicle departure to pick up the first request exactly on time
		assert(first_action_data.get_arrival_time() - travel_time_to_start >= plan.get_operating_start());
		plan.set_departure_time(first_action_data.get_arrival_time() - travel_time_to_start);

		// compute plan arrival time
		if(this->context.darp_instance_configuration()->is_return_to_depot()) {
			unsigned int travel_time = this->context.travel_time_provider()->get_travel_time(
				plan.get_last_action().get_node(), plan.get_vehicle().get_init_position());
			plan.set_arrival_time(plan.get_servicing_end() + travel_time);
		}
	    else {
		    plan.set_arrival_time(plan.get_servicing_end());
	    }
	}

private:
	Temporal_action_bounds compute_temporal_action_bounds(
		const IH_vehicle_plan_builder<V, A, P>& plan,
		index_in_plan existing_action_count
	) const {
		Temporal_action_bounds bounds;
		bounds.earliest_service_starts.resize(existing_action_count);
		bounds.latest_service_starts.resize(existing_action_count);

		for(index_in_plan i = 0; i < existing_action_count; ++i) {
			const A& current_action = plan[i];
			std::int64_t earliest_arrival;
			if(i == 0) {
				earliest_arrival = static_cast<std::int64_t>(plan.get_operating_start())
					+ this->context.travel_time_provider()->get_travel_time(
						plan.get_vehicle().get_init_position(),
						current_action.get_node()
					);
			}
			else {
				const A& previous_action = plan[i - 1];
				earliest_arrival = bounds.earliest_service_starts[i - 1]
					+ previous_action.get_service_duration()
					+ this->context.travel_time_provider()->get_travel_time(
						previous_action.get_node(),
						current_action.get_node()
					);
			}
			bounds.earliest_service_starts[i] = std::max<std::int64_t>(
				current_action.get_min_time(),
				earliest_arrival
			);
		}

		for(index_in_plan i = existing_action_count - 1; i >= 0; --i) {
			const A& current_action = plan[i];
			if(i == existing_action_count - 1) {
				bounds.latest_service_starts[i] = current_action.get_max_time();
			}
			else {
				const A& next_action = plan[i + 1];
				const std::int64_t latest_before_next =
					bounds.latest_service_starts[i + 1]
					- current_action.get_service_duration()
					- this->context.travel_time_provider()->get_travel_time(
						current_action.get_node(),
						next_action.get_node()
					);
				bounds.latest_service_starts[i] = std::min<std::int64_t>(
					current_action.get_max_time(),
					latest_before_next
				);
			}

			if(i == 0) {
				break;
			}
		}
		return bounds;
	}

	const DARP_context<N> context;
};

