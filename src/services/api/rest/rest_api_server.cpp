#include "rest_api_server.h"
#include <services/service_manager.h>
#include <fmt/format.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/exceptions.hpp>
#include <boost/algorithm/string.hpp>
#include <functional>
#include "utility/json_utils.h"

#define REQUEST_RESPONSE_PARAMS const std::shared_ptr<HttpServer::Response>& response, const std::shared_ptr<HttpServer::Request>& request

using namespace boost::property_tree;
using namespace SimpleWeb;

// Helper function for parsing an int ID from the request path
int getIntPathParam(const std::shared_ptr<HttpServer::Request>& request, int position) {
    std::istringstream idStream(request->path_match[position].str());
    int id;
    idStream >> id;
    if (idStream.fail()) {
        throw std::runtime_error("Failed to parse integer from request path.");
    } else {
        return id;
    }
}

void sendSuccessResponse(const std::shared_ptr<HttpServer::Response>& response, std::string content) {
    SimpleWeb::CaseInsensitiveMultimap commonHeader;
    commonHeader.emplace("Access-Control-Allow-Origin", "*");
    commonHeader.emplace("Content-Type", "application/json");
    response->write(StatusCode::success_ok,content,commonHeader);
}

void sendFailureResponse(const std::shared_ptr<HttpServer::Response>& response, std::string content) {
    SimpleWeb::CaseInsensitiveMultimap commonHeader;
    commonHeader.emplace("Access-Control-Allow-Origin", "*");
    commonHeader.emplace("Content-Type", "application/json");
    response->write(StatusCode::client_error_bad_request,content,commonHeader);
}

// Helper functions used to handle requests
void handleStatus(REQUEST_RESPONSE_PARAMS) {
    sendSuccessResponse(response, "ONLINE");
}

void handleSystemState(REQUEST_RESPONSE_PARAMS) {
    if (SM::getSystemRecordManager()->isSystemRunning()) {
        sendSuccessResponse(response, "{\"systemEnabled\" : true }");
    } else {
        sendSuccessResponse(response, "{\"systemEnabled\" : false }");
    }
}

void handleSystemEnable(REQUEST_RESPONSE_PARAMS) {
    SM::getSystemRecordManager()->startSystem();
    handleSystemState(response, request);
}

void handleSystemDisable(REQUEST_RESPONSE_PARAMS) {
    SM::getSystemRecordManager()->stopSystem();
    handleSystemState(response, request);
}

void handleSetActiveSystem(REQUEST_RESPONSE_PARAMS) {
    try {
        int id = getIntPathParam(request, 1);
        SM::getSystemRecordManager()->setActiveSystem(id);
        sendSuccessResponse(response, fmt::format("{{\"activeSystem\" : {} }}", id));
    } catch (std::exception &error){
        response->write(StatusCode::client_error_bad_request, error.what());
    }
}

void handleGetActiveSystem(REQUEST_RESPONSE_PARAMS) {
    record_system_t activeSystem = SM::getSystemRecordManager()->getActiveSystem();
    sendSuccessResponse(response, fmt::format("{{\"activeSystem\" : {} }}", activeSystem.id));
}

void handleGetSystems(const std::shared_ptr<HttpServer::Response>& response, const std::shared_ptr<HttpServer::Request>& request) {
    try {
        boost::property_tree::ptree pt;
        boost::property_tree::ptree list;

        const std::vector<record_system_t> &systems = SM::getSystemRecordManager()->getSystems();
        if (systems.empty()) {
            sendSuccessResponse(response, "{\"systems\" : []}");
            return;
        }

        for (const record_system_t &system : systems) {
            list.push_back(std::make_pair("", record_system_t::toPropertyTree(system)));
        }
        pt.add_child("systems",list);
        sendSuccessResponse(response, propertyTreeToString(pt));
    } catch (std::exception &error){
        response->write(StatusCode::client_error_bad_request, error.what());
    }
}

void handleGetSystem(const std::shared_ptr<HttpServer::Response>& response, const std::shared_ptr<HttpServer::Request>& request) {
    try {
        int id = getIntPathParam(request, 1);
        record_system_t system = SM::getSystemRecordManager()->getSystem(id);
        std::string jsonString = propertyTreeToString(record_system_t::toPropertyTree(system));
        sendSuccessResponse(response, jsonString);
    } catch (std::exception &error){
        response->write(StatusCode::client_error_bad_request, error.what());
    }
}

void handleCreateSystem(const std::shared_ptr<HttpServer::Response>& response, const std::shared_ptr<HttpServer::Request>& request) {
    try {
        SM::getLogger()->info(fmt::format("Creating system from json [{}]",request->content.string()));
        boost::property_tree::ptree pt = stringToPropertyTree(request->content.string());
        record_system_t record = SM::getSystemRecordManager()->createSystem(pt.get<std::string>("name"));
        std::string jsonString = propertyTreeToString(record_system_t::toPropertyTree(record));
        sendSuccessResponse(response, jsonString);
    } catch (std::exception &error){
        SM::getLogger()->error(error.what());
        sendFailureResponse(response, error.what());
    }
}

void handleUpdateSystem(const std::shared_ptr<HttpServer::Response>& response, const std::shared_ptr<HttpServer::Request>& request) {
    try {
        int id = getIntPathParam(request, 1);
        boost::property_tree::ptree pt = stringToPropertyTree(request->content.string());
        SM::getSystemRecordManager()->updateSystem({
            id,
            pt.get<std::string>("name")
        });
        response->write(StatusCode::success_ok);
    } catch (std::exception &error){
        response->write(StatusCode::client_error_bad_request, error.what());
    }
}

void genericOptionsResponse(const std::shared_ptr<HttpServer::Response>& response, const std::shared_ptr<HttpServer::Request>& request) {
    SimpleWeb::CaseInsensitiveMultimap commonHeader;
    commonHeader.emplace("Access-Control-Allow-Origin", "*");
    commonHeader.emplace("Access-Control-Allow-Headers", "*");
    commonHeader.emplace("Content-Type", "application/json");
    response->write(StatusCode::success_ok,commonHeader);
}

void handleDeleteSystem(const std::shared_ptr<HttpServer::Response>& response, const std::shared_ptr<HttpServer::Request>& request) {
    try {
        SM::getSystemRecordManager()->deleteSystem(getIntPathParam(request, 1));
        response->write(StatusCode::success_no_content);
    } catch (std::exception &error){
        response->write(StatusCode::client_error_bad_request, error.what());
    }
}

RestApiServer::RestApiServer() {
    // Configure server options
    server = new HttpServer();

    server->config.port = SM::getConfig()->getInt("rest_api_port",9000);

    // Register functions
    server->resource["^/v1/status"]["GET"] = handleStatus;
    server->resource["^/v1/systems"]["GET"] = handleGetSystems;
    server->resource["^/v1/system"]["POST"] = handleCreateSystem;
    server->resource["^/v1/system"]["OPTIONS"] = genericOptionsResponse;
    server->resource["^/v1/system/([0-9]+)$"]["GET"] = handleGetSystem;
    server->resource["^/v1/system/([0-9]+)$"]["PUT"] = handleUpdateSystem;
    server->resource["^/v1/system/([0-9]+)$"]["DELETE"] = handleDeleteSystem;
    server->resource["^/v1/system_state"]["GET"] = handleSystemState;
    server->resource["^/v1/system_state_enable"]["GET"] = handleSystemEnable;
    server->resource["^/v1/system_state_disable"]["GET"] = handleSystemDisable;
    server->resource["^/v1/system/([0-9]+)/set_active$"]["GET"] = handleSetActiveSystem;
    server->resource["^/v1/system/get_active$"]["GET"] = handleGetActiveSystem;

    server->on_error = [](const std::shared_ptr<HttpServer::Request>& request, const SimpleWeb::error_code & ec) {
        if (ec != SimpleWeb::errc::operation_canceled) {
            SM::getLogger()->error(fmt::format("Error occurred while handling request {}: {}",request->query_string, ec.message()));
        }
    };

    serverThread = std::thread{&RestApiServer::_runServerThread, this};
}

RestApiServer::~RestApiServer() {
    SM::getLogger()->info("Stopping REST API Server...");
    server->stop();
    serverThread.join();
    delete server;
    SM::getLogger()->info("Stopped REST API Server.");
}

void RestApiServer::_runServerThread() {
    server->start([](unsigned short port) {
        SM::getLogger()->info(fmt::format("Started REST API server on port {}.", port));
    });
}