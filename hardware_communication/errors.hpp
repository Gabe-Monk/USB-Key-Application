#ifndef ERRORS_HPP
#define ERRORS_HPP

#include <stdio.h>
#include <map>
#include <string>

// #define DEBUG // Comment out this line to disable debug logs

typedef enum {
    OK                     = 0,
    ERROR_GENERIC          = -1,
    ERROR_FAILED_HANDSHAKE = -2,
    ERROR_SERIAL_PORT      = -3,
    ERROR_INVALID_LENGTH   = -4,
    ERROR_UNRECOGNIZED     = -5,
    ERROR_INIT             = -6,
    ERROR_FINGERPRINT      = -7,
    ERROR_AUTH_REQ         = -8
} deviceErr;

std::string deviceErrToStr(const deviceErr err) {
    static const std::map<deviceErr, std::string> errMap = {
        {OK, "OK"},
        {ERROR_GENERIC, "ERROR_GENERIC"},
        {ERROR_FAILED_HANDSHAKE, "ERROR_FAILED_HANDSHAKE"},
        {ERROR_SERIAL_PORT, "ERROR_SERIAL_PORT"},
        {ERROR_INVALID_LENGTH, "ERROR_INVALID_LENGTH"},
        {ERROR_UNRECOGNIZED, "ERROR_UNRECOGNIZED"},
        {ERROR_INIT, "ERROR_INIT"},
        {ERROR_FINGERPRINT, "ERROR_FINGERPRINT"},
        {ERROR_AUTH_REQ, "ERROR_AUTH_REQ"}
    };
    
    auto it = errMap.find(err);
    if (it != errMap.end()) {
        return it->second;
    }
    return "UNKNOWN_ERROR_CODE";
}

#define LOG_ERR(err, errStr, ...) fprintf(stderr, "%s - %s (%s:%d) - " errStr "\n", deviceErrToStr(err).c_str(), __func__, __FILE__, __LINE__, ##__VA_ARGS__);
#define LOG_ERROR(errStr, ...) fprintf(stderr, "ERROR - %s (%s:%d) - " errStr "\n", __func__, __FILE__, __LINE__, ##__VA_ARGS__);

#ifdef DEBUG
    #define LOG_DBG(dbgStr, ...) printf("DEBUG - %s - " dbgStr "\n", __func__, ##__VA_ARGS__);
#else
    #define LOG_DBG(dbgStr, ...) ((void)0)
#endif

#define CHK(func) do { \
        deviceErr _ret = (func); \
        if (_ret != OK) { \
            fprintf(stderr, "ERROR (%d, %s) - %s:%d (%s)\n", _ret, deviceErrToStr(_ret).c_str(), __FILE__, __LINE__, __func__); \
            return _ret; \
        } \
    } while (0)

#endif