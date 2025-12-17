#ifndef WIFI_SSID
  #error "WIFI_SSID is not defined. Set env var WIFI_SSID and rebuild."
#endif

#ifndef WIFI_PASS
  #error "WIFI_PASS is not defined. Set env var WIFI_PASS and rebuild."
#endif


// Function Prototypes
bool initializeWebServer();
