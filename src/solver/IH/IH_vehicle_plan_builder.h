#pragma once

#include "../Vehicle_plan_builder.h"
#include "../../Request.h"


template<class V, Vehicle_plan_builder_action A, Vehicle_plan_builder_plan<V, A> P>
class IH_vehicle_plan_builder: public Vehicle_plan_builder<V, A, P> {

public:
	IH_vehicle_plan_builder(const V& vehicle_par, plan_size_type init_size, unsigned operating_start = 0):
		Vehicle_plan_builder<V, A, P>(vehicle_par, init_size, init_size * 4 + 2, operating_start),
		operating_start(operating_start)
	{}

	template<Vehicle_plan_builder_action_with_request AR = A>
	explicit IH_vehicle_plan_builder(const P& plan);


	[[nodiscard]] plan_size_type get_action_data_used_length() const{
		return action_data_used_length;
	}

	void set_cost_before_pickup() {
		this->cost_before_pickup = this->get_cost();
	}

	void set_cost_before_drop_off() {
		this->cost_before_drop_off = this->get_cost();
	}

	
	[[nodiscard]] plan_size_type compute_time_adjustment_start_index() const {
		if(cost_before_drop_off == 0) {
			return 0;
		}
		
	    return 2 + (action_data_used_length - 1) * 2;
    }

	/**
	 * @brief The length of the plan including the action we are currently adding.
	 * The action on get_active_length() - 1 can thus be null
	 * @return the active length of the plan.
	*/
	[[nodiscard]] plan_size_type get_active_length() const {
		// empty plan
		if(action_data_used_length == 0) {
			return 0;
		}
		// we are adding the drop off action
		if(cost_before_drop_off) {
			return action_data_used_length;
		}
		// we are adding the pickup action
		else {
			return action_data_used_length - 1;
		}
	}

	[[nodiscard]] plan_size_type get_completed_length() const {
		// empty plan
		if(get_active_length() == 0) {
			return 0;
		}

		if(this->action_order[get_active_length()] == -1) {
			return get_active_length() - 1;
		}

		return get_active_length();
	}

	const A& get_last_action() const {
        assert(get_active_length() > 0);
        return this->action_data[this->action_order[get_active_length() - 1]];
    }

	/**
    * @brief Returns the end time of the request servicing: the departure time of the last action.
	*/
    unsigned int get_servicing_end() const {
        return get_last_action().get_departure_time();
    }

	[[nodiscard]] time_type get_operating_start() const {
		return operating_start;
	}

	[[nodiscard]] const std::vector<time_type>& get_earliest_service_starts() const {
		return earliest_service_starts;
	}

	[[nodiscard]] const std::vector<time_type>& get_latest_service_starts() const {
		return latest_service_starts;
	}

	[[nodiscard]] bool has_valid_temporal_action_bounds() const {
		return has_temporal_action_bounds_for_length(action_data_used_length);
	}

	[[nodiscard]] bool has_temporal_action_bounds_for_length(plan_size_type length) const {
		return earliest_service_starts.size() == length
			&& latest_service_starts.size() == length;
	}

	void set_temporal_action_bounds(
		std::vector<time_type>&& earliest_service_starts_par,
		std::vector<time_type>&& latest_service_starts_par
	) {
		assert(earliest_service_starts_par.size() == action_data_used_length);
		assert(latest_service_starts_par.size() == action_data_used_length);
		earliest_service_starts = std::move(earliest_service_starts_par);
		latest_service_starts = std::move(latest_service_starts_par);
	}

	void clear_temporal_action_bounds() {
		earliest_service_starts.clear();
		latest_service_starts.clear();
	}

	void add_new_request_data(A& pickup_action_data, A& drop_off_action_data) {
		assert(action_data_used_length <= this->action_data.size());
		action_data_used_length += 2;

		// action data size needs to be raised (this plan has been the best plan recently)
		if(action_data_used_length > this->action_data.size()) {
			this->action_data.push_back(pickup_action_data);
			this->action_data.push_back(drop_off_action_data);
			//this->reference_action_data();
		}
		else {
			this->action_data[action_data_used_length - 2] = pickup_action_data;
			this->action_data[action_data_used_length - 1] = drop_off_action_data;
		}

		// action order and time adjustments size needs to be raised (this plan has been the best plan recently, and the initial size was exceeded)
		if(action_data_used_length > this->action_order.size()) {
			this->action_order.push_back(-1);
			this->action_order.push_back(-1);
			this->time_adjustments.resize(action_data_used_length * 4 + 2);
		}
		
		//pickup-drop off referencing
		this->action_data[action_data_used_length - 1].set_other_action_data_index(
			static_cast<index_in_plan>(action_data_used_length - 2));
		this->action_data[action_data_used_length - 2].set_other_action_data_index(
			static_cast<index_in_plan>(action_data_used_length - 1));

		// we need to erase this, as it is a drop off indicator
		cost_before_drop_off = 0; 
	}

	void remove_new_request_data() {
		action_data_used_length -= 2;
	}

	void remove_lastly_added_action(bool pickup, bool revert_cost = false) {
		assert(action_data_used_length > 0);

    	// cost reset
    	if(revert_cost){
	        this->set_cost(pickup ? cost_before_pickup : cost_before_drop_off);
        }

    	// time adjustments rollback
        const auto start_index = this->compute_time_adjustment_start_index();
    	if(this->time_adjustments[start_index]) {
    		this->departure_time -= this->time_adjustments[start_index + 1];
    		auto time_adjustment_action_index = start_index + 2;
	        for(plan_size_type i = 0; i < action_data_used_length - 1; ++i) {
	        	A& action_data_to_rollback = this->action_data[i];

	        	// arrival time
//				assert(this->time_adjustments[time_adjustment_action_index] >= 0);
	        	int time_adjustment = this->time_adjustments[time_adjustment_action_index];
	        	if(time_adjustment > 0){
	        		assert(time_adjustment <= static_cast<int>(action_data_to_rollback.get_arrival_time()));
					action_data_to_rollback.set_arrival_time(action_data_to_rollback.get_arrival_time() - time_adjustment);
	        		this->time_adjustments[time_adjustment_action_index] = 0;
                }
	        	++time_adjustment_action_index;

	        	// departure time
	        	time_adjustment = this->time_adjustments[time_adjustment_action_index];
	        	if(time_adjustment > 0){
	        		assert((int) time_adjustment <= action_data_to_rollback.get_departure_time());
	        		assert(static_cast<int>(action_data_to_rollback.get_departure_time()) - time_adjustment
						>= static_cast<int>(action_data_to_rollback.get_arrival_time()));
					action_data_to_rollback.set_departure_time(action_data_to_rollback.get_departure_time() - time_adjustment);
	        		this->time_adjustments[time_adjustment_action_index] = 0;
                }
	        	++time_adjustment_action_index;
	        }
    		this->time_adjustments[start_index] = 0;
    		this->time_adjustments[start_index + 1] = 0;
    	}

		A& lastly_added_action_data
			= pickup ? this->action_data[action_data_used_length - 2] : this->action_data[action_data_used_length - 1];
    	
		/*
		 * action order reverting
		 */ 
		index_in_plan position_in_plan = lastly_added_action_data.get_position_in_plan();

		// if the index_in_plan is -1, we do not need to rollback the order, as it has not been set yet.
		if(position_in_plan >= 0){
			const unsigned last_action_index = pickup ? action_data_used_length - 2 : action_data_used_length - 1;
			if(static_cast<unsigned>(position_in_plan) == last_action_index) { // it was last action
				this->action_order[position_in_plan] = -1;
			}
			else {
				for(unsigned i = position_in_plan; i < last_action_index; ++i) {
					this->action_order[i] = this->action_order[i + 1];
					this->operator[](i).set_position_in_plan(static_cast<index_in_plan>(i));
				}
				// this should be redundant
				if(pickup) {
					this->action_order[action_data_used_length - 2] = -1;
				}
				else {
					this->action_order[action_data_used_length - 1] = -1;
				}
			}
			lastly_added_action_data.set_position_in_plan(-1);
		}
		
		lastly_added_action_data.delete_departure_time();
    	lastly_added_action_data.set_arrival_time(0);

		//cost before drop off needs to be set to zero, because it is an indicator
		if(!pickup) {
			cost_before_drop_off = 0; 
		}
	}

	void erase_time_adjustments() {
		std::fill(this->time_adjustments.begin(), this->time_adjustments.end(), 0);
	}


private:
	/**
	 * @brief Currently used size of the action_data vector
	*/
	plan_size_type action_data_used_length{0};

	/*
	 * Cost of the plan before the pickup action was added.
	 */
	unsigned long cost_before_pickup{0};

	/*
	 * Cost of the plan before the drop off action was added.
	 */
	unsigned int cost_before_drop_off{0};

	time_type operating_start;

	std::vector<time_type> earliest_service_starts;

	std::vector<time_type> latest_service_starts;

};


#include "IH_vehicle_plan_builder.tpp"
