//
// Created by SG Lee on 2019/11/01.
//

#ifndef SEGMENT_MEASUREMENT_SEARCH_ATTRIBUTEMANAGER_H
#define SEGMENT_MEASUREMENT_SEARCH_ATTRIBUTEMANAGER_H

#include <map>
#include <string>
#include "Attribute.h"

using namespace std;
class AttributeManager {
private:
    AttributeManager() {}
    ~AttributeManager() {}
    AttributeManager(const AttributeManager &) = delete;
    AttributeManager &operator=(const AttributeManager &) = delete;
public:
    inline static AttributeManager &get_instance() {
        static AttributeManager instance;
        return instance;
    }
private:
    map<string, Attribute *> attribute_map;
public:
    bool insert_attribute(const string &key, Attribute *attribute) {
        pair<map<string,Attribute *>::iterator, bool> ret =
                this->attribute_map.insert(
                        pair<const string, Attribute *>(
                                key,
                                attribute
                        ));
        return ret.second;
    }
    Attribute *get_attribute(const string &key)const {
        map<string,Attribute *>::const_iterator citr =
                this->attribute_map.find(key);
        return (citr!=this->attribute_map.end())? citr->second : nullptr;
    }
};

#endif //SEGMENT_MEASUREMENT_SEARCH_ATTRIBUTEMANAGER_H
