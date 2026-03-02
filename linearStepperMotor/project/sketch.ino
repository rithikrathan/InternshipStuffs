/*
 * Stepper Motor Controller with Oscillation, Positioning, and Timers
 * Updated for A4988 Driver (STEP/DIR interface)
 *
 * Commands:
 * $<rpm>                 - Set speed (e.g., $15, Max 20)
 * start                  - Begin oscillating
 * stop                   - Return to origin
 * moveToPos <cm>         - Move to exact position and stop (e.g., moveToPos 5.5)
 * setStartTime <seconds> - Schedule 'start' in X seconds
 * setStopTime <seconds>  - Schedule 'stop' in X seconds
 * status                 - Show current state
 * getPos                 - Show position in cm
 */

#define DIR_PIN 6  // Connect to A4988 DIR pin
#define STEP_PIN 7 // Connect to A4988 STEP pin

#define WHEEL_RADIUS_CM 9 // Radius of the wheel attached to the stepper
#define TRAVEL_DISTANCE_CM 200.0 // Total distance to oscillate in cm
#define DEFAULT_RPM 4 // Default speed in RPM

// NEMA 17 motors are typically 200 steps per revolution. 
// Note: If you use microstepping (e.g., 1/16th on the A4988), multiply this by your microstep value (e.g., 200 * 16 = 3200).
const int stepsPerRotation = 200; 

// State machine
enum StepperState {
  STATE_IDLE,
  STATE_RUNNING,
  STATE_STOPPING,
  STATE_HALTED,
  STATE_MOVING_TO_POS
};

class StepperController {
  private:
    StepperState currentState;
    int currentPositionSteps;
    int targetSpeedRpm;
    bool runningDirection;
    
    // Position targeting
    int targetPositionSteps;

    // Timer variables
    bool startTimerActive;
    unsigned long startDelayMs;
    unsigned long startTimerStartMs;

    bool stopTimerActive;
    unsigned long stopDelayMs;
    unsigned long stopTimerStartMs;

    // Timing tracking for the A4988 steps
    unsigned long lastStepTimeUs;

    float stepsToCm(int steps) {
      float circumference = 2.0 * PI * WHEEL_RADIUS_CM;
      return (steps * circumference) / stepsPerRotation;
    }

    int cmToSteps(float cm) {
      float circumference = 2.0 * PI * WHEEL_RADIUS_CM;
      return (cm * stepsPerRotation) / circumference;
    }

    int getLimitSteps() {
      return cmToSteps(TRAVEL_DISTANCE_CM);
    }

    // Custom step method for A4988
    void stepMotor(int stepDir) {
      // Calculate microsecond delay between steps based on RPM
      unsigned long stepDelayUs = 60000000UL / (stepsPerRotation * targetSpeedRpm);
      
      // Wait until it is time for the next step to maintain correct RPM
      while (micros() - lastStepTimeUs < stepDelayUs) {
        // block and wait
      }
      lastStepTimeUs = micros();

      // Set Direction
      if (stepDir > 0) {
        digitalWrite(DIR_PIN, HIGH);
      } else {
        digitalWrite(DIR_PIN, LOW);
      }

      // Fire Step Pulse
      digitalWrite(STEP_PIN, HIGH);
      delayMicroseconds(2); // A4988 requires a minimum 1 microsecond high pulse
      digitalWrite(STEP_PIN, LOW);
    }

  public:
    StepperController() : currentState(STATE_IDLE), 
      currentPositionSteps(0), 
      targetSpeedRpm(DEFAULT_RPM),
      runningDirection(true),
      targetPositionSteps(0),
      startTimerActive(false), stopTimerActive(false),
      lastStepTimeUs(0) {}

    void init() {
      pinMode(STEP_PIN, OUTPUT);
      pinMode(DIR_PIN, OUTPUT);
      printMenu();
    }

    void printMenu() {
      Serial.println("\n=== Stepper Motor Controller ===");
      Serial.println("Commands:");
      Serial.println("  $<rpm>                 - Set speed (e.g., $15, Max 20)");
      Serial.println("  start                  - Begin oscillating");
      Serial.println("  stop                   - Return to origin");
      Serial.println("  moveToPos <cm>         - Move to exact position and stop (e.g., moveToPos 5.5)");
      Serial.println("  setStartTime <seconds> - Schedule 'start' in X seconds");
      Serial.println("  setStopTime <seconds>  - Schedule 'stop' in X seconds");
      Serial.println("  status                 - Show current state");
      Serial.println("  getPos                 - Show position in cm");
      Serial.println("================================\n");
    }

    void parseCommand(String cmd) {
      cmd.trim();

      if (cmd.startsWith("$")) {
        int rpm = cmd.substring(1).toInt();
        // You can comfortably increase this max limit for a NEMA 17 / A4988 setup
        if (rpm > 0 && rpm <= 100) { 
          targetSpeedRpm = rpm;
          Serial.print("Speed set to: ");
          Serial.println(targetSpeedRpm);
        } else {
          Serial.println("Invalid RPM! Must be between 1 and 20.");
        }
        return;
      }

      if (cmd == "start") {
        if (currentState == STATE_HALTED || currentState == STATE_MOVING_TO_POS) {
          currentState = STATE_IDLE;
        }
        if (currentState == STATE_IDLE) {
          currentState = STATE_RUNNING;
          runningDirection = true;
          Serial.println("Started: Moving forward (Oscillating)");
        } else {
          Serial.println("Already running or stopping");
        }
        return;
      }

      if (cmd == "stop") {
        if (currentState == STATE_RUNNING || currentState == STATE_MOVING_TO_POS) {
          currentState = STATE_STOPPING;
          Serial.println("Stopping: Returning to origin");
        } else if (currentState == STATE_IDLE || currentState == STATE_HALTED) {
          Serial.println("Already at origin / stopped");
        } else {
          Serial.println("Already stopping");
        }
        return;
      }
      
      if (cmd.startsWith("moveToPos ")) {
        float targetCm = cmd.substring(10).toFloat();
        targetPositionSteps = cmToSteps(targetCm);
        currentState = STATE_MOVING_TO_POS;
        Serial.print("Moving to exact position: ");
        Serial.print(targetCm);
        Serial.println(" cm");
        return;
      }

      if (cmd.startsWith("setStartTime ")) {
        float seconds = cmd.substring(13).toFloat();
        startDelayMs = seconds * 1000;
        startTimerStartMs = millis();
        startTimerActive = true;
        Serial.print("Timer: Will START oscillating in ");
        Serial.print(seconds);
        Serial.println(" seconds.");
        return;
      }

      if (cmd.startsWith("setStopTime ")) {
        float seconds = cmd.substring(12).toFloat();
        stopDelayMs = seconds * 1000;
        stopTimerStartMs = millis();
        stopTimerActive = true;
        Serial.print("Timer: Will STOP (return to origin) in ");
        Serial.print(seconds);
        Serial.println(" seconds.");
        return;
      }

      if (cmd == "status") {
        printStatus();
        return;
      }

      if (cmd == "getPos") {
        Serial.print("Position: ");
        Serial.print(stepsToCm(currentPositionSteps));
        Serial.println(" cm");
        return;
      }

      Serial.println("Unknown command");
    }

    void printStatus() {
      Serial.print("State: ");
      switch (currentState) {
        case STATE_IDLE: Serial.println("IDLE"); break;
        case STATE_RUNNING: Serial.println("RUNNING (Oscillating)"); break;
        case STATE_STOPPING: Serial.println("STOPPING"); break;
        case STATE_HALTED: Serial.println("HALTED"); break;
        case STATE_MOVING_TO_POS: Serial.println("MOVING TO POS"); break;
      }
      Serial.print("Speed: ");
      Serial.println(targetSpeedRpm);
      Serial.print("Position: ");
      Serial.print(stepsToCm(currentPositionSteps));
      Serial.println(" cm");
      if (startTimerActive) {
        Serial.print("Start Timer Active: ");
        Serial.print((startDelayMs - (millis() - startTimerStartMs)) / 1000.0);
        Serial.println("s remaining");
      }
      if (stopTimerActive) {
        Serial.print("Stop Timer Active: ");
        Serial.print((stopDelayMs - (millis() - stopTimerStartMs)) / 1000.0);
        Serial.println("s remaining");
      }
    }

    void checkTimers() {
      if (startTimerActive && (millis() - startTimerStartMs >= startDelayMs)) {
        startTimerActive = false;
        Serial.println("\n[Timer Triggered: START]");
        parseCommand("start");
      }

      if (stopTimerActive && (millis() - stopTimerStartMs >= stopDelayMs)) {
        stopTimerActive = false;
        Serial.println("\n[Timer Triggered: STOP]");
        parseCommand("stop");
      }
    }

    void update() {
      checkTimers();

      int limitSteps = getLimitSteps();

      switch (currentState) {
        case STATE_IDLE:
        case STATE_HALTED:
          break;

        case STATE_RUNNING:
          if (runningDirection) {
            if (currentPositionSteps < limitSteps) {
              stepMotor(1);
              currentPositionSteps++;
            } else {
              runningDirection = false;
              Serial.println("Reached +limit, reversing");
            }
          } else {
            if (currentPositionSteps > -limitSteps) {
              stepMotor(-1);
              currentPositionSteps--;
            } else {
              runningDirection = true;
              Serial.println("Reached -limit, reversing");
            }
          }
          break;

        case STATE_STOPPING:
          if (currentPositionSteps > 0) {
            stepMotor(-1);
            currentPositionSteps--;
          } else if (currentPositionSteps < 0) {
            stepMotor(1);
            currentPositionSteps++;
          } else {
            currentState = STATE_HALTED;
            Serial.println("Stopped at origin");
          }
          break;

        case STATE_MOVING_TO_POS:
          if (currentPositionSteps < targetPositionSteps) {
            stepMotor(1);
            currentPositionSteps++;
          } else if (currentPositionSteps > targetPositionSteps) {
            stepMotor(-1);
            currentPositionSteps--;
          } else {
            currentState = STATE_HALTED;
            Serial.println("Reached target position. Holding.");
          }
          break;
      }
    }
};

// main code
StepperController controller;

void setup() {
  Serial.begin(9600);
  controller.init();
}

void loop() {
  controller.update();
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    controller.parseCommand(cmd);
  }
}