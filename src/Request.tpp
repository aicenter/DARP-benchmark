//
// Created by Fido on 2020-04-02.
//

template<typename N>
const Service_action<N> &Request<N>::get_pickup() const {
    return pickup;
}

template<typename N>
const Service_action<N> &Request<N>::get_dropoff() const {
    return drop_off;
}

template<typename N>
unsigned short Request<N>::get_min_travel_time() const {
    return min_travel_time;
}

template<typename N>
request_index_type Request<N>::get_index() const {
    return index;
}



template<typename N>
Request<N>::Request(unsigned int index, unsigned int pickup_id, unsigned int dropoff_id,
    std::shared_ptr<N> pickup_node, unsigned int pickup_min_time, unsigned int pickup_max_time,
    std::shared_ptr<N> dropoff_node, unsigned int dropoff_min_time, unsigned int dropoff_max_time,
    unsigned short min_travel_time, unsigned short pickup_service_time, unsigned short dropoff_service_time)
    : index{index}, pickup(pickup_node, pickup_id, pickup_min_time, pickup_max_time, Action_type::pickup, *this,
                            pickup_service_time),
        drop_off(dropoff_node, dropoff_id, dropoff_min_time, dropoff_max_time, Action_type::dropoff, *this,
                              dropoff_service_time),
        min_travel_time{min_travel_time}{}

template <typename N>
Request<N>::Request(Request&& other) noexcept:
	index(other.index),
	pickup(std::move(other.pickup)),
	drop_off(std::move(other.drop_off)),
	min_travel_time(other.min_travel_time)
{
    pickup.request = this;
    drop_off.request = this;
}



//hash function for Request and unordered_set<Request>
template<typename N>
struct std::hash<const Request<N>> {
	std::size_t operator()(const Request<N> &r) const {
		using std::size_t;
		using std::hash;
		return (hash<unsigned long>()(r.get_index()));
	}
};

template<typename N>
void Request<N>::JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    writer.StartObject();
    writer.Key("index");
    writer.Uint(index);
    writer.Key("pickup");
    pickup.JSON_serialize(writer);
    writer.Key("drop_off");
    drop_off.JSON_serialize(writer);
    writer.Key("min_travel_time");
    writer.Uint(min_travel_time);
    writer.EndObject();
}

