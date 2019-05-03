# circuit_demo
ESP8266 Circuit integration demo

# Instructions
This example has only been tested on HiLetgo ESP8266 NodeMCU LUA CP2102 ESP-12E board.
* Download and install Arduino IDE for your OS from https://www.arduino.cc/en/Main/Software
* Set up your Arduino IDE as: Go to File->Preferences and copy the URL below to get the ESP board manager extensions: http://arduino.esp8266.com/stable/package_esp8266com_index.json 
* Go to Tools > Board > Board Manager> Type "esp8266" and download the Community esp8266 and install. 
* Set up your chip as:
    - Tools -> Board -> NodeMCU 1.0 (ESP-12E Module)
    - Tools -> Flash Size -> 4M (3M SPIFFS)
    - Tools -> CPU Frequency -> 80 Mhz
    - Tools -> Upload Speed -> 921600
    - Tools-->Port--> (whatever it is)
    
* Needed Libraries for Client
    - WiFiManager
    - ArduinoJson
