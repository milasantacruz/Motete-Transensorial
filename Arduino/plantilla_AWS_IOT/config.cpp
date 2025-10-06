#include "config.h"

WiFiConfig wifiConfig = {
    .ssid = "FreakStudio_TPLink",
    .password = "Freaknoize2025"
};

// Configuración AWS IoT Core
AWSConfig awsConfig = {
    .endpoint = "a38a842oqvrvuj-ats.iot.us-east-2.amazonaws.com",  // ⚠️ CAMBIAR por tu endpoint real
    .port = 8883,
    .thingName = "ESP82_Client",
    .caCert = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF",
    .deviceCert = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDWTCCAkGgAwIBAgIUZCBW4k2EQzGCDSF+sOtYhIH+X/UwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI1MTAwNTIzMDcz
NVoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMD28TiJx2rAIwIpjhII
HoT2KJ7w6pITvIkQbmsGwKPi2gZfKD7BiTPt1waaEsr6tBG1uqEmJYBu88NTiPu6
H6Aq+XMbH3tSyj+/1vH/KjfHIfmBHGbw56VV0CeebAmm8kXuGLHdXJiHGdkFd3sK
PCIEZM0L2SmFcur3EhPUmv5Pz36ojhkWCZaJcRkV5m+zQClVXUZSbum+2Th1qWVo
4XOonXYGZAselRPpk7vO0KEt2uB0fzpy3EVVIvpLfKJd0t/e9iUBOx+F8dueMDlN
P29VOLPynUVViM99SVT7HDMNtnQcSzI2T/fqb3RQjv27d/9VFJv32RZQMSAHrA0f
x4MCAwEAAaNgMF4wHwYDVR0jBBgwFoAUbcjCYPr2UU6eN59uAmXaY2NqbNcwHQYD
VR0OBBYEFN7ckWeIp6g8VPXqouWMboeD2pvvMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQAGvdaHL+2pPSNZTS0WqtKaEO9o
Hc3oqCJa0er3g3zcqL0F/VXp8FyTgv40w9qJVM/gUfRkzmfGpbUIZMQEM/3BRO78
EMRWXNYScM4Hh1Z+woHSM558+LH8roy5D3ndgE3Dsni4CJoSU/joQmEyHd8wG3GP
Mxd7a+R5di6yrJWfJ34b02yEIZC71B8YsHBDq1dFKNYxEh3ITxqZtL+hFqyIn14y
wjldV5mhdbGQFTMEDWxIRpuPWIJ3xWxc/5t6WhyOhJzARhCCWdSKL+SaPnMPFp8p
COVwEqRv72xU2bWJZ169EPig1Y/WaD/TyQzwqz1MgnV1QVVN82pXLGM11tli
-----END CERTIFICATE-----
)EOF",
    .privateKey = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEAwPbxOInHasAjAimOEggehPYonvDqkhO8iRBuawbAo+LaBl8o
PsGJM+3XBpoSyvq0EbW6oSYlgG7zw1OI+7ofoCr5cxsfe1LKP7/W8f8qN8ch+YEc
ZvDnpVXQJ55sCabyRe4Ysd1cmIcZ2QV3ewo8IgRkzQvZKYVy6vcSE9Sa/k/PfqiO
GRYJlolxGRXmb7NAKVVdRlJu6b7ZOHWpZWjhc6iddgZkCx6VE+mTu87QoS3a4HR/
OnLcRVUi+kt8ol3S3972JQE7H4Xx254wOU0/b1U4s/KdRVWIz31JVPscMw22dBxL
MjZP9+pvdFCO/bt3/1UUm/fZFlAxIAesDR/HgwIDAQABAoIBAESbR/aHHgy5VXke
VzHYCynQ725bT7syIzVET03kL7fVHxlm2cM+Qk7hQDNLsmTSsBUf3MQQ8QJx8F9B
IxIFAduLi/c97HqHW/tAdObDhzofpde07Ok4u9OP6Bs9+0GPRtJQLh2HC7X7Sthb
Vji70ddyYnh4vvRBSzN5NscwLw89qCFORLIGlPPyl75ZRxA3du20Bv6wGfou4KFB
YvS+n9NuYVLeklUQMwU/A8SbNkawG00qHBcvpX6PbyV4RIb/28dGzMIyCxGkcYLJ
TaRqq0J+wYkrAqt4Y+98xyuO4G+EZ+hweb4p/1iN8RGKj9DXMcekt1IdA7rNAPyB
2zm3TKECgYEA90Y0m1zMPtgnaQQ1L1o3YbZUTK7PQu/FqYbOLYdf+7sHy9PwYGh1
3wAulDcys1z2RDZrKihu53ijWeFFNQ9qzSLgam4oKrc1+fVSmI5tYoaVITsDRhe8
/R8V2PrGVeJ4bLDGEV7K62vz0NOA83O1cLF+dqR1HcC51KZyE7Usk5ECgYEAx8Yf
FlcLDvQ494OqvoQ2aOPkexl+knIi0RjwiCNSOhvsXaTxlIrYWTO2VCFg62YvBOEb
p73fHxte5PuhdIGpvm7zJCdzyUW4hAKFpCbBrNc8yjRAWM9RmbLKk0DWY246FVCA
BzE8xVJ0wCSk1SYT7koK4YaQbxEnh7wZanfON9MCgYEAtq3+NX1UQ2VrkDEUnbwX
yC9sjg5jfr8nF3xJG+e0aIQfWV1la8QguLQotUUmRs55/aD6gPXIIWfFvqQnffC6
5XxLsTVapVwfG2A7OguXEj/9MsSnQYROEe6CNH/oQREECMB8Q971KQHi6bcnwDQO
qofmtv3+rBFy+IBTLO4fVZECgYBYEgPKeX0qjog+thEkBG1oOG+VxVuDfEXVIMWJ
fujVLVI6xmiqL7vJN379/+kWXeoEoKjsfkxLDmn7UOOQ1Ujb51XRboMhDFeX+vSn
tV5UsF7gVcC4Zk13ENc1q7PYCdQZaW/Hu/EqnHtT6dOxFCRdPM8nNYefL1TaBha+
VSrYeQKBgQDr34y6yEeaRZkYTB1Mkz4x4kTO3TlU9ZTTUAROnyZd4BPz1Xjo/pif
phPd3G31Nr6lYqQIcOlsh5tIEgBjtCcEZ4zwDUBmHV9fgazRhT20yjJmxMsR9QNw
gqMQXA5NbVXka2bcVGzCJFMx2i8tJ4kZHfwgB4Az6fMCsxiG/bZ8rQ==
-----END RSA PRIVATE KEY-----
)EOF",
    .qos = 1,
    .keepAlive = 60,
    .cleanSession = true
};

MQTTConfig mqttConfig = {
    .server = "192.168.1.34",
    .port = 1883,
    .user = "osmo_norte",
    .password = "norte",
    .clientId = "osmo_norte",
    .qos = 1, // QoS 1 para garantizar entrega
    .keepAlive = 60,   
    .cleanSession = true
};

DeviceConfig deviceConfig = {
    .unitId = "ESP82",  // ✅ Cambiado para coincidir con AWS Thing Name
    .pumpCount = 4,  // ✅ Cambiado a 4 para tener bombas 0, 1, 2, 3
    .statusInterval = 60000,  // ✅ Aumentado a 60 segundos para reducir carga
    .pumpPins = {12,13,14,15},  // Pines más seguros para ESP8266
    .pumpDefaults = {
        .activationTime = 2000,  // 10 segundos por defecto
        .cooldownTime = 3000     // 30 segundos por defecto
    }
};