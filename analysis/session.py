import paho.mqtt.client as mqtt
import csv
import json
import time
from collections import deque
import matplotlib.pyplot as plt
import matplotlib.animation as animation

fig, (ax1, ax2) = plt.subplots(1, 2)


start_time = time.time()
rms_buffer = deque(maxlen=50) 
raw_buffer = deque(maxlen=50) 


def update(frame):
    ax1.clear()
    ax1.plot(list(raw_buffer), color="grey")
    ax1.set_xlabel("Sample")
    ax1.set_ylabel("ADC value")
    ax1.set_title("Raw (rectified)")
    ax1.set_ylim(0, 4096)

    ax2.clear()
    ax2.plot(list(rms_buffer), color="blue")
    ax2.set_xlabel("Sample")
    ax2.set_ylabel("RMS")
    ax2.set_title("Processed (RMS)")
    ax2.set_ylim(0, 4096)



ani = animation.FuncAnimation(
	    fig,            # the figure to animate
	    update,         # function called on every frame
	    interval=100,   # milliseconds between frames (100ms = 10 fps)
	    cache_frames=False  # don't store frames in memory
	)



def elapsed_ms():
    return int((time.time() - start_time) * 1000)

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        client.subscribe([
            ("emg/esp32_01/features", 1),
            ("emg/esp32_01/raw", 1),
        ])

def on_message(client, userdata, msg):
    t_ms = elapsed_ms()
    try:
        data = json.loads(msg.payload.decode())
    except json.JSONDecodeError:
        return

    if msg.topic == "emg/esp32_01/features":
        rms = data.get("rms")
        if rms is not None:
            userdata["writer"].writerow([t_ms, "rms", round(rms, 6)])
            rms_buffer.append(rms)
            print(f"t={t_ms:8d}ms   rms={rms:.4f}", end="\r")

    elif msg.topic == "emg/esp32_01/raw":
        raw = data.get("raw")
        if raw is not None:
            userdata["writer"].writerow([t_ms, "raw", raw])

def start_logger(writer):
    client = mqtt.Client(userdata={"writer": writer}) 
    client.on_connect = on_connect
    client.on_message = on_message
    client.will_set("emg/esp32_01/status", "offline", qos=1, retain=True)
    client.connect("localhost", 1883, keepalive=60)
    client.loop_start()  

if __name__ == "__main__":
    csv_file = open("session_features.csv", "w", newline="")
    writer = csv.writer(csv_file)
    writer.writerow(["timestamp_ms", "type", "value"])  
    
    try:
        start_logger(writer)
        plt.show()         
    except KeyboardInterrupt:
        print("\nStopped")
    finally:
        csv_file.flush()
        csv_file.close()

