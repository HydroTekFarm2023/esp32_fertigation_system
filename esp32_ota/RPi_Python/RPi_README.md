Host name - raspberrypi
login as - pi
password - raspberry

For installing python version 3
1. sudo apt update
2. sudo apt install python3 idle3

Update ~/.bashrc to use python3 (in case of multiple python version installed)
1. alias python='/usr/bin/python3.7'
2. source ~/.bashrc
3. check python version - python -V

To install mosquitto broker
- sudo apt install -y mosquitto mosquitto-clients
- To check mosquitto service status - sudo service mosquitto status
- To stop mosquitto service - sudo service mosquitto stop

To install paho-mqtt (1.5.1) on raspberryPi Enter below cmd.
- pip3 install paho-mqtt python-etcd

To install apache server on Raspberry Pi
	- sudo apt update
	- sudo apt install apache2
	- Port can be configured from `/etc/apache2/ports.conf` file in Ubuntu
	- Apache2 service can be start/restart using command(s): `sudo service apache2 start` OR `sudo service apache2 restart` 

To change permission of /var/www/html
- sudo chmod 777 /var
- sudo chmod 777 /var/www
- sudo chmod 777 /var/www/html

Put IP address of Raspberry Pi into python script
- publish_cmd = "http://<RaspberryPi IP address>/mqtt_ota.bin"

Change wait period for ESP OTA complete ACK
- timer = threading.Timer(<time in seconds to wait>, timer_expire)
 
How to use Raspberry Pi script
- Change Raspberry Pi IP address in script
- Change wait period for ESP OTA complete ACK
- Run Script
	python mqtt_raspberry_script.py
- Publish download start command for Raspberry Pi script on topic "/fw_upgrade/download_start"
	Currently we have used Mosquitto Pub utility to test this.
	mosquitto_pub -m "https://csespota.s3.ap-south-1.amazonaws.com/hello-world.bin" -t "/fw_upgrade/download_start" -h <Raspberry Pi IP>
- Script will wait till wait period for ESP OTA ACK and create /home/pi/mac.txt file before terminating
- If OTA is failed for any ESP device, we will have entry in mac.txt file as - "MAC address of ESP WiFi STA:ota_failed"
- If OTA is successful for any ESP device, we will have entry in mac.txt file as - "MAC address of ESP WiFi STA:ota_success".


