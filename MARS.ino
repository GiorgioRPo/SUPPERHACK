// (c) Michael Schoeffler 2017, http://www.mschoeffler.de

//#include "Wire.h" // This library allows you to communicate with I2C devices.
#include <Adafruit_MPU6050.h>
#include <WiFi.h> // Library connect esp32 to WiFi
#include <FirebaseESP32.h> // Library to work with firebase (FirebaseESP32 Client by Mobizt)
#include "addons/TokenHelper.h" // Addon to provide token generation process info
#include "addons/RTDBHelper.h" // Addon to provide RTDB (Real Time Database) info

Adafruit_MPU6050 mpu1;

const char* WIFI_SSID = "SUTD_Guest";
const char* WIFI_PASSWORD = "";

// Firebase project credentials
#define DB_URL "https://wth-supperhack-default-rtdb.asia-southeast1.firebasedatabase.app/"  // Replace with your Firebase Database URL
#define API_KEY "AIzaSyDP4Ih3FO4GrQxxlPqgFGnHJNS3sYZE4Vc" // Replace with your Firebase API Key or Database Secret

// Firebase and WiFi objects
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;
int x = 0;


#define ACCEL_X 0
#define ACCEL_Y 1
#define ACCEL_Z 2
#define GYRO_X 3
#define GYRO_Y 4
#define GYRO_Z 5

/*
#define ACCEL_Z 0
#define GYRO_Y 1
#define GYRO_Z 2
*/
float a_cal[6] = {0,0,0,0,0,0};
float a_num[6] = {0,0,0,0,0,0};
float a[6] = {0,0,0,0,0,0};
float a0[6] = {0,0,0,0,0,0};
float dv[6] = {0,0,0,0,0,0};
/*float v[3] = {0, 0, 0};
float v0[3] = {0, 0, 0};
float s[3] = {0, 0, 0};
float s0[3] = {0, 0, 0};*/

int calibrating_millis = 0;
const int calibration_time = 3000;

void setup() {
  Serial.begin(9600);

  // Connecting to WIFI
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");

  // Initialize Firebase
  config.api_key = API_KEY;
  config.database_url = DB_URL;
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("Sign Up OK");
    signupOK = true;
  }
  else {
    Serial.println(config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Initialize the Accelerometer
  if (!mpu1.begin()) {
    Serial.println("Could not find MPU6050 at 0x68!");
  }

  // Configure the sensor
  mpu1.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu1.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu1.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println("MPU6050 Initialized!");
}

void loop() {
  sensors_event_t accel, gyro, temper;

  // Get sensor readings
  mpu1.getEvent(&accel, &gyro, &temper);

  // Print accelerometer data
  Serial.print("Accel X: "); Serial.print(accel.acceleration.x); Serial.print(", ");
  Serial.print("Y: "); Serial.print(accel.acceleration.y); Serial.print(", ");
  Serial.print("Z: "); Serial.print(accel.acceleration.z);
  Serial.print(" || Gyro X: "); Serial.print(gyro.acceleration.x); Serial.print(", ");
  Serial.print("Y: "); Serial.print(gyro.acceleration.y); Serial.print(", ");
  Serial.print("Z: "); Serial.println(gyro.acceleration.z);
  
  int delay_millis = 50;

  // Write data to Firebase
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > delay_millis || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    if (Firebase.getString(firebaseData, "Calibrating")) {
      Serial.print("Calibrating: ");
      Serial.println(firebaseData.stringData());
      if (firebaseData.boolData() == true) {
        Serial.println("Calibrating........ ");

        for (int i = 0; i < 6; i += 1)
        {
          a_num[i] += 1;
        }
        a_cal[ACCEL_X] += accel.acceleration.x;
        a_cal[ACCEL_Y] += accel.acceleration.y;
        a_cal[ACCEL_Z] += accel.acceleration.z;
        a_cal[GYRO_X] += gyro.acceleration.x;
        a_cal[GYRO_Y] += gyro.acceleration.y;
        a_cal[GYRO_Z] += gyro.acceleration.z;

        calibrating_millis += delay_millis;
        delay(delay_millis);
        if (calibrating_millis >= calibration_time) {
          calibrating_millis = 0;
          for (int i = 0; i < 6; i += 1) // Set A0, V0, S0 to values of previous iteration
          {
            a_cal[i] = a_cal[i] / float(a_num[i]);
            a_num[i] = 0;
            a[i] = 0;
            a0[i] = 0;
            dv[i] = 0;
            //v[i] = 0;
            //v0[i] = 0;
            //s[i] = 0;
            //s0[i] = 0;
          }
          
          if (Firebase.setBool(firebaseData, "Calibrating", false)) {
            Serial.println(); Serial.print(false);
            Serial.print(" -- successfully saved to: " + firebaseData.dataPath());
            Serial.println(" (" + firebaseData.dataType() + ")");
          }
          else {
            Serial.println("FAILED: " + firebaseData.errorReason());
          }
        }
        return;
      }
      else {
        calibrating_millis = 0;
      }
    }
    else {
      Serial.print("Failed to read value: ");
      Serial.println(firebaseData.errorReason());
    }

    for (int i = 0; i < 6; i += 1) // Set A0, V0, S0 to values of previous iteration
    {
      a0[i] = a[i];
      /*v0[i] = v[i];
      s0[i] = s[i];*/
    }

    // Set new acceleration values
    a[ACCEL_X] = accel.acceleration.x- a_cal[ACCEL_X];
    a[ACCEL_Y] = accel.acceleration.y- a_cal[ACCEL_Y];
    a[ACCEL_Z] = accel.acceleration.z - a_cal[ACCEL_Z];
    a[GYRO_X] = gyro.acceleration.x- a_cal[GYRO_X];
    a[GYRO_Y] = gyro.acceleration.y - a_cal[GYRO_Y];
    a[GYRO_Z] = gyro.acceleration.z - a_cal[GYRO_Z];

    for (int i = 0; i < 6; i += 1) // Set S & V values for current iteration
    {
      find_dv(delay_millis, a[i], a0[i], dv[i]);
    }
    
    if (Firebase.setFloat(firebaseData, "deltaV/dv_x", dv[ACCEL_X])) {
      Serial.println(); Serial.print(dv[ACCEL_X]);
      Serial.print(" -- successfully saved to: " + firebaseData.dataPath());
      Serial.println(" (" + firebaseData.dataType() + ")");
    }
    else {
      Serial.println("FAILED: " + firebaseData.errorReason());
    }
    
    if (Firebase.setFloat(firebaseData, "deltaV/dv_y", dv[ACCEL_Y])) {
      Serial.println(); Serial.print(dv[ACCEL_Y]);
      Serial.print(" -- successfully saved to: " + firebaseData.dataPath());
      Serial.println(" (" + firebaseData.dataType() + ")");
    }
    else {
      Serial.println("FAILED: " + firebaseData.errorReason());
    }
    
    if (Firebase.setFloat(firebaseData, "deltaV/dv_z", dv[ACCEL_Z])) {
      Serial.println(); Serial.print(dv[ACCEL_Z]);
      Serial.print(" -- successfully saved to: " + firebaseData.dataPath());
      Serial.println(" (" + firebaseData.dataType() + ")");
    }
    else {
      Serial.println("FAILED: " + firebaseData.errorReason());
    }
    
    if (Firebase.setFloat(firebaseData, "deltaV/dw_x", dv[GYRO_X])) {
      Serial.println(); Serial.print(dv[GYRO_X]);
      Serial.print(" -- successfully saved to: " + firebaseData.dataPath());
      Serial.println(" (" + firebaseData.dataType() + ")");
    }
    else {
      Serial.println("FAILED: " + firebaseData.errorReason());
    }
    
    if (Firebase.setFloat(firebaseData, "deltaV/dw_y", dv[GYRO_Y])) {
      Serial.println(); Serial.print(dv[GYRO_Y]);
      Serial.print(" -- successfully saved to: " + firebaseData.dataPath());
      Serial.println(" (" + firebaseData.dataType() + ")");
    }
    else {
      Serial.println("FAILED: " + firebaseData.errorReason());
    }
    
    if (Firebase.setFloat(firebaseData, "deltaV/dw_z", dv[GYRO_Z])) {
      Serial.println(); Serial.print(dv[GYRO_Z]);
      Serial.print(" -- successfully saved to: " + firebaseData.dataPath());
      Serial.println(" (" + firebaseData.dataType() + ")");
    }
    else {
      Serial.println("FAILED: " + firebaseData.errorReason());
    }
    
  }
  delay(delay_millis);  // Delay for readability
}

void find_dv(const int& deltaT, const float& a, const float& a0, float& dv)
{
  dv = (1.0/2.0) * (a + a0) * deltaT;
  Serial.println(dv);
  return;
}