#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdint.h>

#include "NetworkMessageTypes.h"

#pragma comment(lib, "Ws2_32.lib")

#include <MemoryMacros.h>