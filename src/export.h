//
// Created by david on 2023-10-27.
//

#pragma once

#include "plan/VehiclePlan.h"

template<typename L>
void export_plan_and_request(
	const Travel_time_provider<L>& travel_time_provider,
	const VehiclePlan<L>& plan,
	const Request<L>& request,
	const std::string& file_path
);

template<typename L>
void export_action(const Action<L>& action, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer);

#include "export.tpp"
