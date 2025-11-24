//
// Created by Fido on 2020-04-02.
//

#pragma once

#include <memory>
#include <string>
#include <rapidjson/document.h>
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

#include "serialization.h"

template <typename N> class Request;

enum class Action_type{pickup, dropoff, depot};

inline std::string action_type_to_string(Action_type action_type){
    if(action_type == Action_type::pickup){
        return "pickup";
    }
    else if(action_type == Action_type::dropoff){
        return "drop_off";
    }
    else {
	    return "depot";
    }
}

inline Action_type action_type_from_string(const std::string& action_type_string){
    if(action_type_string == "pickup"){
        return Action_type::pickup;
    }
    else if(action_type_string == "drop_off"){
        return Action_type::dropoff;
    }
    else {
	    return Action_type::depot;
    }
}

template <typename T>
concept Dereferenceable = requires(T t) {
	*t;
};

template <class T>
struct remove_any_pointer {
	using type = T;
};

template <Dereferenceable T>
struct remove_any_pointer<T> {
	using type = decltype(*std::declval<T>());
};

template <typename T>
using remove_any_pointer_t = typename remove_any_pointer<T>::type;

/**
 * Base class for actions. It is a common base class for both benchmark and test actions.
 * @tparam N node type. It can be both value and pointer type
 **/
template <typename N>
class Action_base {
public:
	using node_ret_type = std::conditional_t<
	    std::is_fundamental_v<N>,
		N,
		const remove_any_pointer_t<N>&
	>;
	/**
	 * Standard constructor
	 * @param node
	 * @param min_time
	 * @param max_time
	 * @param action_type
	 */
	Action_base(const N node, const unsigned int min_time, const unsigned int max_time, const Action_type action_type)
		: node(node), min_time(min_time), max_time(max_time), action_type(action_type) {}

	/**
	 * JSON deserialization constructor
	 * @param json_data
	 */
	Action_base(const rapidjson::GenericValue<rapidjson::UTF8<>>& json_data, N node);

//	template<typename T = N, std::enable_if_t<Dereferenceable<T>>>
//	[[nodiscard]] node_ret_type get_node() const;
//
//	template<typename T = N, std::enable_if_t<!Dereferenceable<T>>>
	[[nodiscard]] node_ret_type get_node() const;

	[[nodiscard]] unsigned int get_min_time() const;

	[[nodiscard]] unsigned int get_max_time() const;

	[[nodiscard]] const Action_type &get_action_type() const;
protected:
	N node;

	/**
     * Min action time in seconds. 0 stands for unconstrained min time.
     */
	unsigned int min_time;

	/**
	 * Max action time in seconds
	 */
	unsigned int max_time;

	Action_type action_type;
};

/**
 * @brief Action class for the DARP benchmark program.
 * @tparam N
 */
template <typename N>
class Action: public Action_base<std::shared_ptr<N>>{
public:
	virtual ~Action() = default;

	Action(
        std::shared_ptr<N> node, 
        unsigned int action_id,
        unsigned int min_time,
        unsigned int max_time, 
        Action_type action_type,
        unsigned short service_time = 0
    );


    Action(const Action& other) = delete;
    Action(Action&& other) noexcept = default;
    Action& operator=(const Action& other) = delete;
    Action& operator=(Action&& other) noexcept = default;
	

    [[nodiscard]] std::shared_ptr<N> get_node_pointer() const;


    [[nodiscard]] unsigned short get_service_duration() const;

    virtual void JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const;

    [[nodiscard]] unsigned int get_action_id() const;

protected:

    const unsigned int action_id;

    const unsigned short service_duration;
};

template<class N>
class Service_action final: public Action<N> {
public:
     Service_action(
		std::shared_ptr<N> node, 
        unsigned int action_id,
        unsigned int min_time, 
        unsigned int max_time, 
        Action_type action_type,
        const Request<N>& request, 
        unsigned short service_time = 0
     );
	
    [[nodiscard]] const Request<N>& get_request() const;

    void JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const override;
private:
	const Request<N>* request;

	friend class Request<N>;
};


static_assert(std::is_same_v<decltype(std::declval<Action_base<int>>().get_node()),int>);
static_assert(std::is_same_v<Action_base<std::string>::node_ret_type,const std::string&>);
static_assert(Dereferenceable<const std::shared_ptr<int>>);

#include "Action.tpp"
