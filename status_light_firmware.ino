// #include <EEPROM.h>

// #include <SpritzCipher.h>

#include "BluetoothSerial.h"
// #include "mbedtls/md.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif


namespace CMD {
	const char END = '\n';
	const String ATTENTION = "AT";
	const String SET_RGB = "SV";
	const String SET_MODE = "SM";
	const String SET_SPEED = "SS";
	const String TOGGLE_ONBOARD_LED = "TL";
};

namespace MODE {
	const String STEADY = "STD";
	const String BLINK = "BLK";
	const String PULSE = "PLS";
};

namespace RESPONSE {
	const String OKAY = "OK";
	const String ERROR = "ER";
};

enum Mode_t {steady, blink, pulse};

struct color_v {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint16_t duration;
	uint16_t speed;
	Mode_t effect;
};


const int LED_RED = 19;
const int LED_BLUE = 21;
const int LED_GREEN = 18;
const int RED_CHANNEL = 0;
const int GREEN_CHANNEL = 1;
const int BLUE_CHANNEL = 2;

BluetoothSerial SerialBT;
Mode_t mode = steady;
color_v colors[32]; // TODO
uint8_t red = 0;
uint8_t green = 0;
uint8_t blue = 0;

// Pulse settings
// uint8_t min_pulse = 0;
// uint8_t max_pulse = 255;
long pulse_duration = 2000;
unsigned long prev_time = millis();
boolean blink_toggle = true;

// uint8_t distance = (max_pulse - min_pulse) / 2;
// uint8_t median = min_pulse + distance;


uint8_t hex_to_int(String hex) {
	char chars[3] = {0, 0, 0};

	hex.toCharArray(chars, 3);

	return constrain(strtol(chars, 0, 16), 0, 255);
}

void cmd_toggle_onboard_led(String data) {
	if (data == "1") {
		digitalWrite(LED_BUILTIN, HIGH);
	} else if (data == "0") {
		digitalWrite(LED_BUILTIN, LOW);
	}
}

void cmd_set_rgb(String data) {
	if (data.length() != 6) {
		return error();
	}

	red = hex_to_int(data.substring(0,2));
	green = hex_to_int(data.substring(2,4));
	blue = hex_to_int(data.substring(4,6));

	if (mode == steady) {
		set_rgb(red, green, blue);
	}

	okay();
}

void cmd_set_mode(String data) {
	if (data.length() != 3) {
		return error();
	}

	if (data == MODE::STEADY) {
		mode = steady;
		set_rgb(red, green, blue);
	} else if (data == MODE::BLINK) {
		mode = blink;
	} else if (data == MODE::PULSE) {
		mode = pulse;
	} else {
		return error();
	}

	okay();
}

void cmd_set_speed(String data) {
	// if (data.length() != 3) {
	// 	return error();
	// }
	Serial.print("Speed: ");
	Serial.println(data);
	Serial.print("converted: ");
	pulse_duration = data.toInt();
	Serial.println(pulse_duration);

	okay();
}

void command_processor(String data) {
	String command;

	data.toUpperCase();

	command = data.substring(0, 2);
	// Serial.println(command);

	if (command == CMD::SET_RGB) {
		cmd_set_rgb(data.substring(2));
	} else if (command == CMD::SET_MODE) {
		cmd_set_mode(data.substring(2));
	} else if (command == CMD::SET_SPEED) {
		cmd_set_speed(data.substring(2));
	} else if (command == CMD::TOGGLE_ONBOARD_LED) {
		cmd_toggle_onboard_led(data.substring(2));
	} else {
		return error();
	}
}

void update_led() {
	if (mode == steady) {
		return;
	} else if (mode == pulse) {
		unsigned long time = millis();
		// uint8_t distance = max_pulse - min_pulse;
		// uint8_t median = min_pulse + (distance / 2);

		// uint8_t v = 128+127*cos(2*PI/pulse_duration*time);
		// uint8_t v = median+distance*cos(2*PI/pulse_duration*time);
		// set_rgb(red, green, v);

		uint8_t r = (red/2) + (red/2) * cos(2 * PI / pulse_duration * time);
		uint8_t g = (green/2) + (green/2) * cos(2 * PI / pulse_duration * time);
		uint8_t b = (blue/2) + (blue/2) * cos(2 * PI / pulse_duration * time);
		set_rgb(r, g, b);
	} else if (mode == blink) {
		unsigned long time = millis();
		if (time - prev_time >= pulse_duration) {
			prev_time = time;
			blink_toggle = !blink_toggle;
			if (blink_toggle) {
				set_rgb(red, green, blue);
			} else {
				set_rgb(0, 0, 0);
			}
		}
	}
}

void set_rgb(uint8_t r, uint8_t g, uint8_t b) {
	ledcWrite(RED_CHANNEL, 256 - r);
	ledcWrite(GREEN_CHANNEL, 256 - g);
	ledcWrite(BLUE_CHANNEL, 256 - b);
}

void okay() {
	SerialBT.print(RESPONSE::OKAY);
	SerialBT.print('\n');
}

void error() {
	Serial.println('error ========================================');
	SerialBT.print(RESPONSE::ERROR);
	SerialBT.print('\n');
}

void setup() {
	Serial.begin(115200);

	ledcAttachPin(LED_RED, RED_CHANNEL);
	ledcAttachPin(LED_GREEN, GREEN_CHANNEL);
	ledcAttachPin(LED_BLUE, BLUE_CHANNEL);
	ledcSetup(RED_CHANNEL, 12000, 8);
	ledcSetup(GREEN_CHANNEL, 12000, 8);
	ledcSetup(BLUE_CHANNEL, 12000, 8);
	// pinMode(LED_BUILTIN, OUTPUT); //Specify that LED pin is output
	set_rgb(0, 0, 100);

	SerialBT.begin("ESP32test"); // Bluetooth device name
	Serial.println("The device started, now you can pair it with bluetooth!");
	// Serial.print("min_pulse: ");
	// Serial.println(min_pulse);
	// Serial.print("max_pulse: ");
	// Serial.println(max_pulse);
	// Serial.print("median: ");
	// Serial.println(median);
	// Serial.print("distance: ");
	// Serial.println(distance);
	// Green LED to signify boot
	set_rgb(0, 100, 0);
	// Turn off LED after a half second
	delay(500);
	set_rgb(0, 0, 0);
}

void loop() {
	String data;
	uint16_t length = 0;

	update_led();

	if (Serial.available()) {
		SerialBT.write(Serial.read());
	}

	if (SerialBT.available()) {
		data = SerialBT.readStringUntil(CMD::END);
		length = data.length();
		// Serial.print("Data: ");
		// Serial.println(data);
		// Serial.print("Length: ");
		// Serial.println(length);

		if (!data.startsWith(CMD::ATTENTION) || length < 3) {
			// delay(10);
			return error();
		}

		if (length != data.charAt(2)) {
			// delay(10);
			return error();
		}

		command_processor(data.substring(3));
	}

	// delay(10);
}
