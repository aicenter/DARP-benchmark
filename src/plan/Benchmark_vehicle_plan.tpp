

//template <typename N>
//void Benchmark_vehicle_plan<N>::JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
//    writer.StartObject();
//    writer.Key("cost");
//    writer.Uint(cost);
//    writer.Key("vehicle");
//    vehicle.get().JSON_serialize(writer);
//    writer.Key("departure_time");
//    writer.Uint(departure_time);
//    writer.Key("arrival_time");
//    writer.Uint(arrival_time);
//    writer.Key("actions");
//    writer.StartArray();
//    for (const ActionData<N>& action : this->actions) {
//        action.JSON_serialize(writer);
//    }
//    writer.EndArray();
//    writer.EndObject();
//}

//template <typename N>
//Benchmark_vehicle_plan<N>::Benchmark_vehicle_plan(const Benchmark_vehicle_plan& other):
//    vehicle(other.vehicle),
//    departure_time(other.departure_time),
//    arrival_time(other.arrival_time)
//{
//	this->actions.reserve(other.actions.capacity());
//    for (const ActionData<N>& action : other.actions) {
//        this->actions.emplace_back(action);
//    }
//}
//
//template <typename N>
//Benchmark_vehicle_plan<N>& Benchmark_vehicle_plan<N>::operator=(const Benchmark_vehicle_plan& other) {
//    if (this == &other)
//        return *this;
//    vehicle = other.vehicle;
//    departure_time = other.departure_time;
//    arrival_time = other.arrival_time;
//
//	this->actions = std::vector<ActionData<N>>();
//    this->actions.reserve(other.actions.capacity());
//    for (const ActionData<N>& action : other.actions) {
//        this->actions.emplace_back(action);
//    }
//
//    return *this;
//}


