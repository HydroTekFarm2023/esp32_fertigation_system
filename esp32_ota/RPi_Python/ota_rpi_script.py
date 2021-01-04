import os
import time
import paho.mqtt.client as mqtt
import urllib.request
import shutil
import threading

#publish_cmd = "http://<OTA_SERVER_IP>:<PORT if needed>/<OTA_BINARY_PATH_IN_SERVER>"
publish_cmd = "http://192.168.0.104/mqtt_ota.bin"
#url = "https://csespota.s3.ap-south-1.amazonaws.com/hello-world.bin"
ota_download_start = 0
fw_download_file = "/var/www/html/mqtt_ota.bin"
mac_address_table = []
mac_file = "/home/pi/mac.txt"

def download_image(url):
    # Download the file from `url` and save it locally under `file_name`:
    with urllib.request.urlopen(url) as response, open(fw_download_file, 'wb') as out_file:
        shutil.copyfileobj(response, out_file)

def on_disconnect(client, userdata, rc):
    print("ESP OTA ack timeout...Disconnecting from MQTT Broker and creating MAC address file: ", mac_file)
    with open(mac_file, 'w') as f:
        for item in mac_address_table:
            f.write("%s\n" % item)
    print("Script END")

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    if rc!=0:
        print("Exit: Bad Connection - result code "+str(rc))
        exit()

    print("Connected to MQTT Broker... Subscribing for OTA start command")
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("/fw_upgrade/download_start")

def timer_expire():
    #print("1 min is completed")
    client.disconnect()

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    #print(msg.topic+" "+str(msg.payload))
    global ota_download_start
    if msg.topic == "/fw_upgrade/download_start":
        if ota_download_start == 1:
            print("OTA already in progress")
        else:
            print("Got msg to download the image from Server")
            ota_download_start = 1
            url = msg.payload.decode('utf-8')
            print(url)
            download_image(url)
            print("Image downloaded from Server...Publishing OTA bin path to ESP")
            client.publish("/fw_upgrade/cmd", publish_cmd)
            client.subscribe("/fw_upgrade/ack")
            print("Waiting for ESP ack...")
            timer = threading.Timer(60, timer_expire)
            timer.start()

    if msg.topic == "/fw_upgrade/ack":
        mac_address_table.append(msg.payload.decode('utf-8'))

print("Script START")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.on_disconnect = on_disconnect

client.connect("localhost", 1883, 60)

# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
client.loop_forever()

