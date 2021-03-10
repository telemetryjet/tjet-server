#include "utility/path_utils.h"

namespace fs = boost::filesystem;

    if (options.contains("filename") && options["filename"].is_string()) {
        filename = options["filename"];
        rawFileOut = true;
    }
    if (options.contains("mode") && options["mode"].is_string()) {
        modeString = options["mode"];
    }

    if (rawFileOut) {
        if (modeString == "overwrite") {
            mode = std::ios::trunc;
        } else if (modeString == "append") {
            mode = std::ios::app;
        } else if (modeString == "new") {
            mode = std::ios::app;
        } else {
            throw std::runtime_error(fmt::format("[{}] data source type '{}' has invalid value for 'mode': {}", id, type, modeString));
        }
        assertDependency("file", filename, fmt::format("[{}] Multiple data sources cannot share the same input/output filename: {}", id, filename));
    }

    if (rawFileOut) {
        // If we are in new file mode, increment filename until the file does not exist
        std::string absPath = resolveRelativePathHome(filename);
        fs::path pathObj(absPath);
        std::string fileName = pathObj.stem().string();
        std::string fileExt = pathObj.extension().string();
        int newFileId = 0;
        if (modeString == "new") {
            do {
                std::string newFilename = fmt::format("{}_{}{}",fileName, newFileId++, fileExt);
                pathObj.remove_filename();
                pathObj.append(newFilename);
            } while (fs::exists(pathObj));
        }
        filename = pathObj.string();
        outputFile = std::ofstream();
        outputFile.open(filename, mode);
        if (!outputFile.is_open()){
            SM::getLogger()->error(fmt::format("[{}] Failed to open file {}", id, pathObj.filename().string()));
            throw std::runtime_error(fmt::format("[{}] Failed to open file {}", id, pathObj.filename().string()));
        } else {
            SM::getLogger()->info(fmt::format("[{}] Opened file {}", id, pathObj.filename().string()));
        }
        flushTimer = std::make_unique<SimpleTimer>(1000);
    }
    if (rawFileOut) {
        if (outputFile.is_open()) {
            outputFile.close();
        }
        flushTimer.reset();
    }

    if (isOnline) {
        if (serial->isOpen()) {
            hasError = false;
            isOnline = true;
            serial->pollBlocking();
        } else {
            hasError = true;
            isOnline = false;
            serial->close();
        }
    } else {
        if (reconnectTimer->check()) {
            // Periodically try to open serial port
            serial->open();
            // Set error flag if we are unable to connect
            hasError = !serial->isOpen();
            isOnline = serial->isOpen();
        }