/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "rush/utils.h"

const char *ConnectivityToString(Connectivity event) {
  switch(event) {
  case kRushConnected:
    return "kRushConnected";
  case kRushConnectionFailed:
    return "kRushConnectionFailed";
  case kRushEstablished:
    return "kRushEstablished";
  case kRushDisconnected:
    return "kRushDisconnected";
  }
  return "";
}
