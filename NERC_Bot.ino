// ########################
// ## GENERAL GUIDELINES ##
// ########################

// Use this "//---##" to signify when you assuming some data in the function that needs to be determined at a much later time
// and you are just using a placeholder and a good estimate or an assumption instead in this case.



// ==========================================
// HARDWARE PIN MAPPING (PLACEHOLDERS)
// ==========================================

//The mapping of these pins right now is completely for
//the sake of placeholding. Redefine them later as your
//needs demand it. 


// --- 1. Front IR Sensor Array ---
// Using analog pins (A0-A2) as digital inputs is common, 
// but you can map these to any digital pin later.
constexpr uint8_t FRONT_IR_LEFT   = A0; 
constexpr uint8_t FRONT_IR_CENTER = A1;
constexpr uint8_t FRONT_IR_RIGHT  = A2;

// --- 2. Back IR Sensor Array (3) ---
// Same as the (1)
constexpr uint8_t BACK_IR_LEFT    = A3;
constexpr uint8_t BACK_IR_CENTER  = A4;
constexpr uint8_t BACK_IR_RIGHT   = A5;

// --- 3. Motor Speed Control (PWM) ---
// The Mega has hardware PWM on pins 2-13 per the schematic. 
// These MUST be connected to the PWM/EN pins on your motor driver.
// What are PWMS? Answer: PWM controls motor speed by rapidly turning a digital 5V signal on and off thousands of times per second.
// The longer the signal stays "on" during each cycle (the duty cycle), the higher the average voltage
// the motor feels, which makes it spin faster. This is mainly done to fake a analog output, actual output is digital.
// Need: Adruino Mega is a digital device, so it can't output analog signal. More on PWM(Pulse Width Modulation): www.arduino.cc/en/Tutorial/PWM&ved=2ahUKEwjG0Yz50P2TAxUnP_sDHY4sNPsQFnoECBoQAQ&usg=AOvVaw2HLv4YoYANcQPx8zRm35mt
constexpr uint8_t MOTOR_LEFT_PWM  = 2;  
constexpr uint8_t MOTOR_RIGHT_PWM = 3;



// --- 4. Motor Direction Control ---
// These dictate forward/reverse. They plug into IN1/IN2/IN3/IN4 
// These pins are to be connected to the L298N. These pins define how to move left
// and right. IN1/IN2 for the left Motor. IN3/IN4 for the right motor.
constexpr uint8_t MOTOR_LEFT_IN1  = 4;
constexpr uint8_t MOTOR_LEFT_IN2  = 5;
constexpr uint8_t MOTOR_RIGHT_IN3 = 6;
constexpr uint8_t MOTOR_RIGHT_IN4 = 7;

// --- 5. More to Come ---
// DO ALL THE HARDWARE MAPPING IN THIS COMMENTED MANNER 
// CLEARLY EXPLAIN WHAT YOU ARE DOING AND WHY WITH REFERENCES IF NECESSARY!



void setup() {

// ==========================================
// DEFINING ALL THE INPUT PINS HERE:
// ==========================================

// Because we need info from our digital IR sensors
// we set them as INPUT pins using the pinmode function (https://docs.arduino.cc/language-reference/en/functions/digital-io/pinMode/)

pinMode(FRONT_IR_LEFT, INPUT);
pinMode(FRONT_IR_CENTER, INPUT);
pinMode(FRONT_IR_RIGHT, INPUT);

// Same goes for the 3 back IR sensros:

pinMode(BACK_IR_LEFT, INPUT);
pinMode(BACK_IR_CENTER, INPUT);
pinMode(BACK_IR_RIGHT, INPUT);


// ==========================================
// DEFINING ALL THE OUTPUT PINS HERE:
// ==========================================

// As explained earlier the purpose of PWM and the nature of it. It is an otput pin.
// It will be mainly used to control the speed of the motor by frequency.
pinMode(MOTOR_LEFT_PWM, OUTPUT);
pinMode(MOTOR_RIGHT_PWM, OUTPUT);

// All the Motor direction control that will go to the motor driver is 
// obviously an output so we will set their pinMode to OUTPUT as well..
pinMode(MOTOR_LEFT_IN1, OUTPUT);
pinMode(MOTOR_RIGHT_IN2, OUTPUT);
pinMode(MOTOR_LEFT_IN3, OUTPUT);
pinMode(MOTOR_RIGHT_IN4, OUTPUT);

// ==========================================
// ALL THE INITIAL WRITING HERE:
// ==========================================

// For Safety we want to ensure that the motors are at rest in the start.
// For that we can use the analogWrite(). More on: https://docs.arduino.cc/language-reference/en/functions/analog-io/analogWrite/

analogWrite(MOTOR_LEFT_PWM, 0);
analogWrite(MOTOR_RIGHT_PWM, 0);



}

void loop() {

	








}

// ==========================================
// HELPR FUNCTIONS GUIDELINES:
// ==========================================
// Try to find ways to pass the parameters as reference to avoid copying cost and saving clockSpeed.
// Iff possible try to write your variables and functions in "constexpr". Details on it: https://share.google/wbv1xGsjTzyfniVUN
// Try to write the code as speed effective as possible, minimizing clock cycles and unnecessary repeats.
// Always prefer simple if-else and for-loop logic for almost all of the task. It is mostly more efficient.
// Properly comment your function, clearly explaining what it does. Why did you make it. Where it is used, reference the line#
// of the codebase where that function is being called.
// More void function, find ways to make any non-void function into void.


// The following function clamps the speed of the driving motor.
// This is necessary to not cause any overflow during our calculation or any mishap 
// to make the moto go crazy.
// Takes any raw integer and strictly bounds it between 0 and 255
// Passing by reference to not make a copy.
void clampPWM(int& raw_speed) {
    if (raw_speed > 255) {
        raw_speed =  255;
    }
    if (raw_speed < 0) {
        raw_speed = 0;
    }

}

// Following functions are gonna be on PD control. This is gonna be more technical but I will
// write the maths and explanation in a diary and attach it. 
// Why not PID (I stands for inegration and D for differentiation)?
// The integration hurts more than helping in our case, it will cause a lot of issues. So I decided to drop it. 
// We will make PD control line following algorithms.

// For Error:

// A global variable so in case we loose all the lines, our robot keeps moving forward based on the last 
// available data instead of js halting.

// Also I am gonna implement the PD line control on the front 3 IR sensors only. The last 3 ones will be to verify 
// whether our turn was a success or not and calibrate in case it wasn't.
// So the all the functions of PD control system assumes the first 3 IR sensors and completely disregard the previous ones.

float last_error = 0;

int calculateError(){

// Read the data from the pins:
	int left_val = digitalRead(FRONT_IR_LEFT);
    int center_val = digitalRead(FRONT_IR_CENTER);
    int right_val = digitalRead(FRONT_IR_RIGHT);

    int sum_active = left_val+center_val+right_val;


    //---## Need to physically calculate the seperation between the 2 IR sensors, for now I am assuming 15 mm.

// Now what I did here needs some explanation. Also for the magic number 15.
// Basically since we are using 3 IR sensors and each sensor is of 15mm in width I added the weights of 15 mm on 
// the value of output given by the sensors. So that is why the middle one is 0 because if it is off we are completely off track
// so we don't need to even calculate it here though I visualized it for readablity. 
// The following example might illustrate:
// If the line shifts slightly right, covering both the Center and Right sensors (0, 1, 1):
//sum_weighted = (0 * -15) + (1 * 0) + (1 * 15) = 15
//current_error = 15 / 2 active sensors = 7.5 (The robot calculates it is exactly 7.5mm off center).



    int weighted_sum = ((left_val*-15) /*+(center_val*0) */ + (right_val*15));



    float current_error;
    
        // 4. Calculate Error or use Memory
        if (sum_active == 0) {
            // LINE LOST: Look at the last known error to guess where it went
            if (last_error < 0) {
                current_error = -15; // Hard left to find it
            } else if (last_error > 0) {
                current_error = 15;  // Hard right to find it
            } else {
                current_error = 0;   // Dead center loss, coast straight
            }
        } else {
            // Calculate standard weighted average
            current_error = (float)weighted_sum / sum_active;
        }
    
        return current_error;
}

// Now that we have the error, we do PD on it. The compute PD function:
// Most of these variable names and thoery comes from system control. I suggested watching: 

// Global tuning variables
float Kp = 2.0;  
float Kd = 1.5;  

float computePD(float current_error) {
    // 1. Calculate Proportional (Present)
    float P = Kp * current_error;

    // 2. Calculate Derivative (Future)
    // How fast is the error changing? 
    float D = Kd * (current_error - last_error);

    // 3. Save the current error for the next loop's memory and derivative
    last_error = current_error; 

    // 4. Return the total motor correction
    return P + D;
}


// Global timing variables for the PD loop
unsigned long last_pd_time = 0;
const int PD_INTERVAL = 10; // 100Hz execution rate

void lineFollow() {
    unsigned long current_time = millis();

    // The function only executes the math if 10ms have passed
    if (current_time - last_pd_time >= PD_INTERVAL) {
        last_pd_time = current_time; // Reset the timer

        // 1. Perception
        float error = calculateError();

        // 2. Thinking
        float correction = computePD(error);

        // 3. Actuation
        driveMotors(correction);
    }
}


// Now write a function to drive the vehicle according the line
// First we declare the base speed of the vehicl at which this is driving.

	//---## We need to adjusst the base_speed based on our hardwawre and arena implementation and all. For now i am assuming 103

int base_speed = 103;
void driveMotors(const int& correction){
// Applying the correction from the computePD function
	int left_speed= base_speed + correction;
	int right_speed= base_speed - correction;

	//Now let's write the code that decides whether both motors should be in forward mode or in the reverse mode

	// Let's start with deciding it for the left motor.
	if(left_speed>=0){
		//if it is positive, we have to tell the motor driver to put the motors in the forward direction. For which:
		digitalWrite(MOTOR_LEFT_IN1, HIGH);
		digitalWrite(MOTOR_LEFT_IN2, LOW);
	} else {
		//Otherwise we need to turn the motors in reverse:
		// Why reverse? in cause of 90 deg turn, we want our vehicles half tires to go back and the other half to go forward
		// This will cause a smooth 90 deg turn **HOPEFULLY**
		digitalWrite(MOTOR_LEFT_IN1, LOW);
		digitalWrite(MOTOR_LEFT_IN2, HIGH);

		//Converting back to a postive number for PWM usage
		left_speed = - left_speed;
	}

	// Now the same logic for the Right Motor.
	if(right_speed>=0){
			//if it is positive, we have to tell the motor driver to put the motors in the forward direction. For which:
			digitalWrite(MOTOR_RIGHT_IN3, HIGH);
			digitalWrite(MOTOR_RIGHT_IN4, LOW);
		} else {
			//Otherwise we need to turn the motors in reverse:
			// Why reverse? in cause of 90 deg turn, we want our vehicles half tires to go back and the other half to go forward
			// This will cause a smooth 90 deg turn **HOPEFULLY**
			digitalWrite(MOTOR_RIGHT_IN3, LOW);
			digitalWrite(MOTOR_RIGHT_IN4, HIGH);
	
			//Converting back to a postive number for PWM usage
			right_speed = - right_speed;
		}

		// Now that we have decided our direction of motor. It is time to 
		// Write the speed to the motors. For our safety, we first clamp all
		// the speed using our clampPWM function.
		clampPWM(right_speed);
		clampPWM(left_speed);

		//Now time to write it to the motors. 
		// This will not work exactly as we have intended because I used a lot of placeholder and assumptions in my
		// Line following code until now. But based on the input of the physical condition. We will be having to adjust the whole
		// code to make as smooth and buttery as possible
		analogWrite(MOTOR_LEFT_PWM, left_speed);
		analogWrite(MOTOR_RIGHT_PWM, right_speed);
}
