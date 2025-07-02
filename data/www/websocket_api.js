var ws_api = {
    reqSystemInfo      : 1,
    reqESP32Reset      : 2,
    reqRA4M1Reset      : 3,    
    repSystemInfo      : 128 + 1, 
    reqGetNetworkParam : 16,
    repGetNetworkParam : 128 + 16,
    reqSetNetworkParam : 17,
    repSetNetworkParam : 128 + 17,    
    reqListFiles       : 32,
    repListFiles       : 128 + 32,
    reqDeleteFiles     : 33,
    repDeleteFiles     : 128 + 33,
}

var ws_wifi_params = {
    ssid : "ssid",
    password : "password",
    hostname :"hostname"
}

var ws_ra4m1_params = {
    reset_status : "ra4m1_reset"
}