/*
 * Copyright (C) Alneos, s. a r. l. (contact@alneos.fr)
 * This file is part of Vega.
 *
 *   Vega is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   Vega is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Vega.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Reference.h
 *
 *  Created on: 15 mai 2014
 *      Author: siavelis
 */

#ifndef REFERENCE_H_
#define REFERENCE_H_

#include <memory>
#include <climits>
#include <boost/functional/hash.hpp>
#include <fstream>
#include <ostream>

namespace vega {

class Model;
/**
 * An object that need to reference to a vega class can use this template to create a reference to an object of that class
 */
template<class T>
class Reference final {
public:
    template<class T2> friend std::string to_str(const Reference<T2>&);
    template<class T2> friend std::ostream& operator<<(std::ostream&, const Reference<T2>&);
    static constexpr int NO_ID = INT_MIN;
    // TODO : make type, original_id and id private
    typename T::Type type;
    int original_id;
    int id;
    Reference(const typename T::Type type, const int original_id = NO_ID, const int id = NO_ID);
    Reference(const T& t);
    bool has_original_id() const {
        return original_id != NO_ID;
    }
    bool has_id() const {
        return id != NO_ID;
    }
    std::shared_ptr<Reference<T>> clone() const;
    ~Reference() {
    }
    ;
};

template<class T>
Reference<T>::Reference(const typename T::Type type, const int original_id, const int id) :
        type(type), original_id(original_id), id(id) {
}

template<class T>
Reference<T>::Reference(const T& t) :
        type(t.type), original_id(t.getOriginalId()), id(t.getId()) {
}

template<class T>
bool operator==(const Reference<T>& left, const Reference<T>& right) {
    if (left.type != right.type) {
        return false;
    }
    if (left.original_id == T::NO_ORIGINAL_ID && right.original_id == T::NO_ORIGINAL_ID) {
        return (left.id == right.id);
    }
    return left.original_id == right.original_id;
}

template<class T>
bool operator <(const Reference<T>& left, const Reference<T>& right) {
    if (left.type != right.type) {
        return left.type < right.type; // first ordering : type
    } else if (left.original_id != T::NO_ORIGINAL_ID && right.original_id != T::NO_ORIGINAL_ID) {
        return left.original_id < right.original_id; // second ordering : original ids
    } else if (left.original_id != T::NO_ORIGINAL_ID || right.original_id != T::NO_ORIGINAL_ID) {
        return left.original_id != T::NO_ORIGINAL_ID; // third ordering : original_id wins over autoid
    } else {
        return left.id < right.id; // last ordering : compare autoids
    }
}

template<class T>
std::string to_str(const Reference<T>& reference) {
    std::ostringstream oss;
    std::string id, type;

    if (reference.has_original_id())
        id = "original_id=" + std::to_string(reference.original_id);
    else
        id = "vega_id=" + std::to_string(reference.id);

    auto it = T::stringByType.find(reference.type);
    if (it != T::stringByType.end())
        type = "type=" + it->second;
    else
        type = "type " + std::to_string(static_cast<int>(reference.type)) + " not yet mapped in stringByType";

    oss << "Reference[" << type << "; " << id << "]";

    return oss.str();
}

template<class T>
std::ostream &operator<<(std::ostream &out, const Reference<T>& reference) {
	out << to_str(reference);
	return out;
}


template<class T>
std::shared_ptr<Reference<T>> Reference<T>::clone() const {
    return std::make_shared<Reference<T>>(*this);
}

}/* namespace vega */

namespace std {

template<class T>
struct hash<vega::Reference<T>> {
    size_t operator()(const vega::Reference<T>& reference) const {
        size_t seed = 0;
        // LD : hash should be different between, for example, Reference<LoadSet> and Reference<ConstraintSet>
        // but LoadSet::Type and ConstraintSet::Type enums both begin with 0 value, so I use the string value instead.
        boost::hash_combine(seed,hash<string>()(T::stringByType.find(reference.type)->second));
        boost::hash_combine(seed,hash<int>()(reference.original_id));
        boost::hash_combine(seed,hash<int>()(reference.id));
        return seed;
    }
};

} /* namespace std */

#endif /* REFERENCE_H_ */
