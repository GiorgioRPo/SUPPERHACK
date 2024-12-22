import pyrebase
import pyautogui
import pydirectinput
import time

# Firebase configuration
firebase_config = {
    "apiKey": "AIzaSyDP4Ih3FO4GrQxxlPqgFGnHJNS3sYZE4Vc",
    "authDomain": "wth-supperhack.firebaseapp.com",
    "databaseURL": "https://wth-supperhack-default-rtdb.asia-southeast1.firebasedatabase.app/",
    "storageBucket": "wth-supperhack.appspot.com"
}

# Initialize Firebase
firebase = pyrebase.initialize_app(firebase_config)
db = firebase.database()

# Mouse sensitivity adjustment
sensitivity = 3

def get_accelerometer_data():
    """Fetch accelerometer data from Firebase."""
    try:
        data = db.child("deltaV").get()
        output_data = {}
        output_data["dv_x"] = data.val().get("dv_x", 0)
        output_data["dv_y"] = data.val().get("dv_y", 0)
        output_data["dv_z"] = data.val().get("dv_z", 0)
        output_data["dw_x"] = data.val().get("dw_x", 0)
        output_data["dw_y"] = data.val().get("dw_y", 0)
        output_data["dw_z"] = data.val().get("dw_z", 0)
        return output_data
    except Exception as e:
        print("Error fetching data:", e)
        return 0, 0

dx, dy = 0, 0

db.child("Calibrating").set(True)
clicked = False

while True:
    data_values = get_accelerometer_data()
    d = {'x':0, 'y':0}

    # Scale the values to move the cursor
    #dx = int((x - x_offset)*sensitivity)
    #dy = int((y - y_offset)*sensitivity)
    calibrating = db.child("Calibrating").get().val()
    if calibrating == True:
        print("CALIBRATING...")
    
    elif calibrating == False:
        if abs(data_values["dv_x"]) >= 30 and data_values["dv_z"] <= -30:
            if data_values["dv_x"] >= 50 and data_values["dv_z"] <= -50 and clicked == False:
                pydirectinput.click(button='right')
                clicked = True
            elif data_values["dv_x"] <= -50 and data_values["dv_z"] <= -50 and clicked == False:
                pydirectinput.click(button='left')
                clicked = True
        else:
            clicked = False
            if data_values["dw_z"] >= 5:
                dx = int((-data_values["dw_z"] - 5) * sensitivity)
            elif data_values["dw_z"] <= -5:
                dx = int((-data_values["dw_z"] + 5) * sensitivity)
            else:
                dx = 0
            
            if data_values["dw_y"] >= 5:
                dy = int((-data_values["dw_y"] - 5) * sensitivity)
            elif data_values["dw_y"] <= -5:
                dy = int((-data_values["dw_y"] + 5) * sensitivity)
            else:
                dy = 0



    # Move the cursor
    pyautogui.moveRel(dx, dy)  # Negative dy to invert Y-axis
    #print(f"gyro_y: {(data_values['gyro_y'])}")
    #print(f"gyro_z: {(data_values['gyro_z'])}")
    #print(f"Mouse moved by ({dx}, {dy})")

    # Adjust the polling frequency
    time.sleep(0.01)
    prev_data_values = data_values
