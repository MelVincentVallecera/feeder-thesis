import firebase_admin
from firebase_admin import credentials
from firebase_admin import db
import datetime
import serial
import time

# Setup Firebase credentials and database
cred = credentials.Certificate("firebase-credentials.json")
firebase_admin.initialize_app(
    cred,
    {
        "databaseURL": "https://fish-feed-fe756-default-rtdb.asia-southeast1.firebasedatabase.app/"
    },
)
sensors = db.reference("sensors")  # Reference to Firebase database node for sensor data
# Setup serial communication with Arduino
ser = serial.Serial(
    port='/dev/ttyACM0', baudrate=115200, timeout=1
)  # Replace with the appropriate port and baud rate

def write_read(x):
    ser.write(bytes(x, 'utf-8'))
    time.sleep(0.05)
    data = ser.readline()
    return data

def check_timer():
    # Get current time
    now = datetime.datetime.now().time()

    # Get list of timer numbers from Firebase database
    timer_numbers = db.reference("timers").get().keys()

    # Check each timer for a match
    for number in timer_numbers:
        timer_time = db.reference("timers/timer" + number + "/time").get()

        # Convert timer time from string to datetime.time object
        timer_time = datetime.datetime.strptime(timer_time, "%H:%M:%S.%f").time()

        # If the timer time matches the current time, send message to Arduino
        if now == timer_time:
            ser.write(b"2\n")
            time.sleep(60)

def check_feednow():
    feednow = db.reference("feednow").get()
    if feednow == 1:
        ser.write(b"1\n")
        db.reference("feednow").set(0)
        time.sleep(60)

while True:
    # Read string of data from Arduino
    data_str = ser.readline().decode('utf-8').strip()

    # Split string into separate values based on commas
    data_list = data_str.split(",")

    # Send values to Firebase database
    sensors.set(
        {
            "ph": str(data_list[0]),
            "dissolvedOxygen": str(data_list[1]),
            "ammonia": str(data_list[2]),
            "waterLevel": str(data_list[3]),
            "waterTemp": str(data_list[4]),
            "feeds": str(data_list[5])
        }
    )
    check_feednow()
    check_timer()
    time.sleep(2)  # Wait 2 seconds before reading again
