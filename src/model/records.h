#ifndef TELEMETRYSERVER_RECORDS_H
#define TELEMETRYSERVER_RECORDS_H

#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/exceptions.hpp>
#include <utility/json_utils.h>

struct record_config_item_t {
    std::string key;
    std::string value;
    static boost::property_tree::ptree toPropertyTree(const record_config_item_t& record) {
        boost::property_tree::ptree pt;
        pt.add("key", record.key);
        pt.add("value", record.value);
        return pt;
    }
};

struct record_system_t {
    int id;
    std::string name;
    static boost::property_tree::ptree toPropertyTree(const record_system_t& record) {
        boost::property_tree::ptree pt;
        pt.add("id", record.id);
        pt.add("name", record.name);
        return pt;
    }
};

struct record_log_t {
    int id;
    int system_id;
    std::string message;
    static boost::property_tree::ptree toPropertyTree(const record_log_t& record) {
        boost::property_tree::ptree pt;
        pt.add("id", record.id);
        pt.add("system_id", record.system_id);
        pt.add("message", record.message);
        return pt;
    }
};

struct record_device_t {
    int id;
    int system_id;
    std::string name;
    static boost::property_tree::ptree toPropertyTree(const record_device_t& record) {
        boost::property_tree::ptree pt;
        pt.add("id", record.id);
        pt.add("system_id", record.system_id);
        pt.add("name", record.name);
        return pt;
    }
};

#endif //TELEMETRYSERVER_RECORDS_H
