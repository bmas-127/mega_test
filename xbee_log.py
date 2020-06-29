import matplotlib.pyplot as plt
import paho.mqtt.client as paho
import numpy as np
import serial
import time

sample_num = 20
t = np.arange(0, sample_num/2 )
ts = np.arange(0, sample_num/2 )
gravity = np.zeros((sample_num, 3))
times = np.zeros((sample_num))
tilt = np.zeros(sample_num)
mqttc = paho.Client()

# Settings mqtt connection
host = "10.0.2.15"
topic= "Mbed"
port = 1883

# Callbacks

def on_connect(self, mosq, obj, rc):

    print("Connected rc: " + str(rc))


def on_message(mosq, obj, msg):

    print("[Received] Topic: " + msg.topic + ", Message: " + str(msg.payload) + "\n");


def on_subscribe(mosq, obj, mid, granted_qos):

    print("Subscribed OK")


def on_unsubscribe(mosq, obj, mid, granted_qos):

    print("Unsubscribed OK")



# XBee setting

serdev = '/dev/ttyUSB0'

s = serial.Serial(serdev, 9600)


s.write("+++".encode())

char = s.read(2)

print("Enter AT mode.")

print(char.decode())


s.write("ATMY 0x109\r\n".encode())

char = s.read(3)

print("Set MY 0x109.")

print(char.decode())


s.write("ATDL 0x209\r\n".encode())

char = s.read(3)

print("Set DL 0x209.")

print(char.decode())


s.write("ATID 0x1\r\n".encode())

char = s.read(3)

print("Set PAN ID 0x1.")

print(char.decode())


s.write("ATWR\r\n".encode())

char = s.read(3)

print("Write config.")

print(char.decode())


s.write("ATMY\r\n".encode())

char = s.read(4)

print("MY :")

print(char.decode())


s.write("ATDL\r\n".encode())

char = s.read(4)

print("DL : ")

print(char.decode())


s.write("ATCN\r\n".encode())

char = s.read(3)

print("Exit AT mode.")

print(char.decode())


print("start sending RPC")

#recieve signal


for i in range(0, 5):
    s.write("/getLog/run\r".encode())
    print("ready")
    time.sleep(1)


print("start to suck dick")

#for i in range (30):
#  line = s.readline()


while True:
    # send RPC to remote
    s.write("/getLog/run\r".encode())
    line=s.readline()
    print(line)
    mqttc.publish(topic, line)
    time.sleep(1)

    
print("sending data to mqtt server")


# Set callbacks

mqttc.on_message = on_message

mqttc.on_connect = on_connect

mqttc.on_subscribe = on_subscribe

mqttc.on_unsubscribe = on_unsubscribe


# Connect and subscribe

print("Connecting to " + host + "/" + topic)

mqttc.connect(host, port=1883, keepalive=60)

mqttc.subscribe(topic, 0)


i = 0
for i in range(0, sample_num):
    mqttc.publish(topic, gravity[i][0])
    mqttc.publish(topic, gravity[i][1])
    mqttc.publish(topic, gravity[i][2])
    mqttc.publish(topic, times[i])
    mqttc.publish(topic, tilt[i])

    time.sleep(0.1)

mqttc.publish(topic, 666)

#draw picture
fig, ax = plt.subplots(2, 1)
ax[0].plot(times,gravity)
ax[1].plot(t, ts, 'bo') 
plt.show()




s.close()