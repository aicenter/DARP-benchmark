//
// Created by Fido on 12/16/2025.
//

#include "../gtest_wrapper.h"
#include "functional_test_helpers.h"

TEST(functional_tests, ih_chyse) {
	fs::path instance_path = get_test_resource_path(fs::path("Chyse") / "config.yaml");

	std::vector<std::string> solver_args = {
		"--method",
		"ih"
	};

	run_functional_test<Amodsim_node>(instance_path, solver_args);
}

