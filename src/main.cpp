#include <services/service_manager.h>
#include <utility/time_utils.h>
#include <fmt/format.h>

/**
 * Main Program Entry Point
 * Starts and runs the server loop.
 */

int exitCode = 0;
bool running = true;

void signalHandler(int signum) {
    running = false;
}

int main() {
    long long startInit = getCurrentMillis();

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Initialize the common services.
    ServiceManager::init();

    // Get the active system, and write some basic data about the setup stats.
    record_system_t activeSystem = SM::getSystemRecordManager()->getActiveSystem();
    SM::getLogger()->info(fmt::format("Active System: [id={},name={}]", activeSystem.id, activeSystem.name));

    SM::getConfig()->setString("systemEnabled","true");

    long long elapsedInitTime = getCurrentMillis() - startInit;

    ServiceManager::getLogger()->info(fmt::format("Started Telemetry Server in {} ms.", elapsedInitTime));

    // Run the server loop
    while (running) {
    }

    ServiceManager::getLogger()->info("Stopping Telemetry Server...");

    // Shutdown the common services
    ServiceManager::destroy();

    // Exit main program
    return exitCode;
}