//
// Created by Fido on 2020-04-22.
//

#include "Coordinate.h"

bool Coordinate::operator==(const Coordinate& other) const {
    if (const Coordinate* c = dynamic_cast<const Coordinate*>(&other)) {
        return this->getX() == c->getX() && this->getY() == c->getY();
    }
    return false;
}

void Coordinate::JSON_serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const {
    writer.StartObject();
    writer.Key("coordinate");
    writer.StartObject();
    writer.Key("x");
    writer.Double(getX());
    writer.Key("y");
    writer.Double(getY());
    writer.EndObject();
    writer.EndObject();
}
