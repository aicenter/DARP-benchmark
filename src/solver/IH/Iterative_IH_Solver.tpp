//
// Created by Fido on 2020-05-14.
//


#include <algorithm>
#include <filesystem>

#include "tqdm.hpp"
#include "Iterative_IH_Solver.h"
#include "Insertion_heuristic_solver.h"

template <typename N>
class Request_pointer_iterator{

public:
    Request_pointer_iterator(std::vector<const Request<N> *>& requests, size_t index) : requests{requests}, index{index} {}

    const Request<N>* begin() const {
        return requests[0];
    }

    const Request<N>* end() const {
        return requests[requests.size()];
    }

    const Request<N>& operator* () const
    {
        return *requests[index];
    }

    const Request_pointer_iterator& operator++ ()
    {
        ++index;
        return *this;
    }

    bool operator!= (const Request_pointer_iterator& other) const
    {
        return index != other.index;
    }

private:
    std::vector<const Request<N>*>& requests;

    size_t index;
};

template <typename N>
class Request_pointer_vector_wrapper{

public:
    explicit Request_pointer_vector_wrapper(std::vector<const Request<N> *>& requests) : requests{requests} {}

    Request_pointer_iterator<N> begin() const {
        return Request_pointer_iterator{requests, 0};
    }

    Request_pointer_iterator<N> end() const {
        return Request_pointer_iterator{requests, requests.size()};
    }

	unsigned int size() const {
	    return (unsigned int) requests.size();
    }
private:
    std::vector<const Request<N>*>& requests;
};


template<typename N>
Iterative_IH_Solver<N>::Iterative_IH_Solver(
	const DARP_instance<N>& instance,
	const DARP_benchmark_config& solver_config,
	const unsigned short iteration_count
)
	: Insertion_heuristic_solver<N>(instance, solver_config, std::filesystem::path{}),
	  iteration_count(iteration_count) {
}

template<typename N>
Solution<N> Iterative_IH_Solver<N>::solve_randomized_iterations() {
    std::vector<const Request<N>*> requests{};
    Request_pointer_vector_wrapper<N> iterator{requests};
    for(const Request<N>& request: this->darp_instance->get_requests()){
        requests.push_back(&request);
    }

    for(unsigned short i : tq::trange(iteration_count)){
        (void) i;
        Solution<N> solution = this->compute(iterator, this->darp_instance->get_vehicles());
        if(!best_solution || solution.get_dropped_request_count() < best_solution.value().get_dropped_request_count()
            || (solution.get_dropped_request_count() == best_solution.value().get_dropped_request_count()
                && solution.get_cost() < best_solution.value().get_cost())){
            best_solution.emplace(std::move(solution));
        }
        std::shuffle(requests.begin(), requests.end(), random_engine);
    }

    return std::move(*best_solution);
}

