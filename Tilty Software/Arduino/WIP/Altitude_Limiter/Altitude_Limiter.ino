#include <i2c_t3.h>

//rc signal include
#include <RCsignal.h>

// IMU includes
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

// Altimeter includes
#include <MPL3115A2.h>

// Compass includes
#include <HMC5883.h>

// SPI mem includes
#include <SpiFlash.h>
#include <SPI.h>

// Sensors
MPU6050 imu;
MPL3115A2 alt;
HMC5883 magn;

// Memory
SpiFlash mem;

//RC Signal
RCsignal RCInput(2, readRC);

// Sensor variables
#define YAW_INDEX 0 // ypr[] data index
#define PITCH_INDEX 1 // ypr[] data index
#define ROLL_INDEX 2 // ypr[] data index
float ypr[3]; // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector
float yaw = 0;
float axyz[3]; // Real world reference acceleration minus gravity
float az_offset = 1.225;
int ix,iy,iz; // compass sensor raw values

float sensor_alt;
float altitude = 0;
float heading;

volatile int throttle;

#include "Servo.h"

Servo throttleOutput;

int altitudeOffset = 0;

void setup() {
	Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);//  Starts I2C on Teensy
	delay(5);
	
	setupDMP();
	alt.init();
	alt.setOversampling(7);
	magn.init();
	
	#ifdef USE_BT
		Serial1.begin(115200);
		#define myPort Serial1
		//while (!myPort.available()) {	readDMP();}
		for (int i = 0; i < 1000; i++) {
			readDMP();
		}
                
		while (myPort.available()) {	myPort.read();}
	#else
		Serial.begin(115200);
		#define myPort Serial
		//while (!myPort) {	readDMP();}
                //mem.begin(10, 2);
                //mem.eraseChip();
		for (int i = 0; i < 1000; i++) {
			readDMP();
			if (Serial) {
				Serial.println("Zeroing. . .");
			}
		}
	#endif
	
	while (!alt.getDataReady()) {}
	altitude = alt.readAltitudeM();
	
	imu.setZAccelOffset(771);

        throttleOutput.attach(23);
        throttleOutput.writeMicroseconds(1000);
}

bool save = false;

bool altSet = false;

void loop(){
  
        if(throttle > 1200 && !altSet)
        {
          altitudeOffset = altitude;
          altSet = true;
        }
        else if(throttle < 1200)
        {
          altitudeOffset = 0;
          altSet = false;
        }
        
	readDMP();
	computeAltitude();
	calculateYaw();

        if(altitude > 5)
        {
          throttleOutput.writeMicroseconds(1200);
        }else throttleOutput.writeMicroseconds(throttle);
	
	if (Serial.available()) {
		Serial.read();
		long start = millis();
		while (millis() - start < 30000) {
			readDMP();
			if (alt.getDataReady()) Serial.println(alt.readAltitudeM()); alt.forceMeasurement();
		}
	}
	
	//if (alt.getDataReady()) Serial.println(alt.readAltitudeM()); alt.forceMeasurement();
        
        if (save) {
          mem.bufferData(altitude);
          mem.bufferData(heading);
          mem.bufferData(axyz[2]);
          mem.bufferData(ypr[PITCH_INDEX]);
        }
        //save = !save;
        /*
        while (Serial) {
          Serial.println("Altitude, Heading, Z Accel, Pitch");
          for (int i = 0; i < mem.getWrittenPages() * 256; i+= 16) {
            Serial.print(mem.readFloat(i));
	    Serial.print(", ");
            Serial.print(mem.readFloat(i+4));
	    Serial.print(", ");
            Serial.print(mem.readFloat(i+8));
	    Serial.print(", ");
            Serial.println(mem.readFloat(i+12));
          }
          Serial.println(az_offset);
          while (true) {}
        }
        */
	/*
	Serial.print("P");
	Serial.println(ypr[PITCH_INDEX]);
	Serial.print("Y");
	Serial.println(yaw);
	Serial.print("R");
	Serial.println(ypr[ROLL_INDEX]);
	Serial.print("A");
	Serial.println(altitude);
	Serial.print("T");
	Serial.println(alt.readTempF());
	*/
}

void readRC()
{
 throttle = RCInput.read(); 
}
