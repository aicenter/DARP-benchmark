#pragma once

#include <type_traits>
#include <vector>

#include "DARP_instance.h"

class Checkable_plan_interface {
public:
	virtual ~Checkable_plan_interface() = default;
	[[nodiscard]] virtual bool check(const DARP_instance_configuration& configuration) const = 0;
};

template<typename P>
concept Checkable_plan = std::is_base_of_v<Checkable_plan_interface, P>;

template<Checkable_plan P>
class Plan_checker {
public:
	explicit Plan_checker(const DARP_instance_configuration& configuration)
		: configuration(configuration) {
	}

	void check_plans(const std::vector<P>& plans) {
		for (const P& plan: plans) {
			check_plan(plan);
		}
	}

	void check_plan(const P& plan) {
        plan.check(configuration);
	}

private:
	const DARP_instance_configuration& configuration;
};