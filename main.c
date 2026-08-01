#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

/* Macros for Control Thresholds */
#define TEMP_MIN        18.0f
#define TEMP_MAX        26.0f
#define HUMIDITY_MAX    65.0f
#define CO2_MAX         1000
#define SMOKE_THRESHOLD 1

/* Enums for System States and Modes */
typedef enum {
    OFF = 0,
    ON = 1
} DeviceState;

typedef enum {
    MODE_AUTOMATIC,
    MODE_MANUAL
} SystemMode;

typedef enum {
    STATUS_NORMAL,
    STATUS_WARNING,
    STATUS_EMERGENCY
} SystemStatus;

/* Sensor Data Structure */
typedef struct {
    float temperature; /* in °C */
    float humidity;    /* in % */
    int co2;           /* in PPM */
    int smoke;         /* 0: Clear, 1: Smoke Detected */
    int motion;        /* 0: Vacant, 1: Occupied */
} SensorData;

/* Actuator Data Structure */
typedef struct {
    DeviceState cooling_fan;
    DeviceState heater;
    DeviceState exhaust_fan;
    DeviceState alarm;
    DeviceState ventilation;
} ActuatorState;

/* HVAC System State Structure */
typedef struct {
    SensorData sensors;
    ActuatorState actuators;
    SystemMode mode;
    SystemStatus status;
} HVAC_System;

/* Function Declarations */
void HVAC_Init(HVAC_System *sys);
void HVAC_ReadSensors(SensorData *sensors);
void HVAC_ProcessLogic(HVAC_System *sys);
void HVAC_DisplayLCD(const HVAC_System *sys);
void HVAC_ManualOverride(HVAC_System *sys);
void Delay(int seconds);

/* Function Definitions */

void HVAC_Init(HVAC_System *sys) {
    sys->mode = MODE_AUTOMATIC;
    sys->status = STATUS_NORMAL;
    
    /* Default actuator state */
    sys->actuators.cooling_fan = OFF;
    sys->actuators.heater = OFF;
    sys->actuators.exhaust_fan = OFF;
    sys->actuators.alarm = OFF;
    sys->actuators.ventilation = OFF;

    /* Initialize random seed */
    srand((unsigned int)time(NULL));
}

void HVAC_ReadSensors(SensorData *sensors) {
    /* Simulate realistic industrial readings */
    sensors->temperature = 15.0f + ((float)rand() / RAND_MAX) * 20.0f; /* 15.0 to 35.0 °C */
    sensors->humidity    = 40.0f + ((float)rand() / RAND_MAX) * 35.0f; /* 40.0 to 75.0 % */
    sensors->co2         = 400 + (rand() % 900);                       /* 400 to 1300 PPM */
    sensors->motion      = rand() % 2;                                 /* 0 or 1 */
    
    /* 10% chance of smoke fault for emergency demonstration */
    sensors->smoke       = ((rand() % 100) < 10) ? 1 : 0;
}

void HVAC_ProcessLogic(HVAC_System *sys) {
    /* Emergency Fault Detection */
    if (sys->sensors.smoke == SMOKE_THRESHOLD) {
        sys->status = STATUS_EMERGENCY;
        sys->actuators.cooling_fan = OFF;
        sys->actuators.heater = OFF;
        sys->actuators.ventilation = OFF;
        sys->actuators.exhaust_fan = ON;
        sys->actuators.alarm = ON;
        return;
    }

    /* Skip automatic logic in Manual Mode */
    if (sys->mode == MODE_MANUAL) {
        sys->status = STATUS_NORMAL;
        return;
    }

    sys->status = STATUS_NORMAL;

    /* Temperature Control Logic */
    if (sys->sensors.temperature > TEMP_MAX) {
        sys->actuators.cooling_fan = ON;
        sys->actuators.heater = OFF;
    } else if (sys->sensors.temperature < TEMP_MIN) {
        sys->actuators.cooling_fan = OFF;
        sys->actuators.heater = ON;
    } else {
        sys->actuators.cooling_fan = OFF;
        sys->actuators.heater = OFF;
    }

    /* Humidity and Air Quality (CO2) Control Logic */
    if (sys->sensors.humidity > HUMIDITY_MAX || sys->sensors.co2 > CO2_MAX) {
        sys->actuators.exhaust_fan = ON;
        if (sys->sensors.co2 > CO2_MAX) {
            sys->status = STATUS_WARNING;
        }
    } else {
        sys->actuators.exhaust_fan = OFF;
    }

    /* Occupancy-Based Ventilation Control Logic */
    if (sys->sensors.motion == 1) {
        sys->actuators.ventilation = ON;
    } else {
        sys->actuators.ventilation = OFF;
    }

    sys->actuators.alarm = OFF;
}

void HVAC_DisplayLCD(const HVAC_System *sys) {
    const char *mode_str = (sys->mode == MODE_AUTOMATIC) ? "AUTO" : "MANUAL";
    const char *status_str = (sys->status == STATUS_NORMAL) ? "NORMAL" :
                             (sys->status == STATUS_WARNING) ? "WARNING" : "EMERGENCY";

    printf("\n==========================================================\n");
    printf("                  INDUSTRIAL HVAC PANEL                   \n");
    printf("==========================================================\n");
    printf(" Mode: %-8s | System Status: %-10s\n", mode_str, status_str);
    printf("----------------------------------------------------------\n");
    printf(" SENSOR READINGS:\n");
    printf("   Temp: %5.1f C   | Humidity: %5.1f %% | CO2: %4d PPM\n", 
           sys->sensors.temperature, sys->sensors.humidity, sys->sensors.co2);
    printf("   Smoke: %-8s | Motion:   %-10s\n", 
           sys->sensors.smoke ? "DETECTED!" : "Clear", 
           sys->sensors.motion ? "Occupied" : "Vacant");
    printf("----------------------------------------------------------\n");
    printf(" ACTUATOR STATUS:\n");
    printf("   Cooling Fan : [%-3s]  | Heater      : [%-3s]\n", 
           sys->actuators.cooling_fan ? "ON" : "OFF", 
           sys->actuators.heater ? "ON" : "OFF");
    printf("   Exhaust Fan : [%-3s]  | Ventilation : [%-3s]\n", 
           sys->actuators.exhaust_fan ? "ON" : "OFF", 
           sys->actuators.ventilation ? "ON" : "OFF");
    printf("   Alarm       : [%-3s]\n", 
           sys->actuators.alarm ? "ON" : "OFF");
    printf("==========================================================\n");
}

void HVAC_ManualOverride(HVAC_System *sys) {
    int choice;
    printf("\n--- MANUAL ACTUATOR CONTROL ---\n");
    printf("1. Toggle Cooling Fan (%s)\n", sys->actuators.cooling_fan ? "ON" : "OFF");
    printf("2. Toggle Heater      (%s)\n", sys->actuators.heater ? "ON" : "OFF");
    printf("3. Toggle Exhaust Fan (%s)\n", sys->actuators.exhaust_fan ? "ON" : "OFF");
    printf("4. Toggle Ventilation (%s)\n", sys->actuators.ventilation ? "ON" : "OFF");
    printf("5. Toggle Alarm       (%s)\n", sys->actuators.alarm ? "ON" : "OFF");
    printf("Enter selection: ");
    
    if (scanf("%d", &choice) != 1) {
        while (getchar() != '\n'); /* Clear input buffer */
        return;
    }

    switch (choice) {
        case 1: sys->actuators.cooling_fan = !sys->actuators.cooling_fan; break;
        case 2: sys->actuators.heater = !sys->actuators.heater; break;
        case 3: sys->actuators.exhaust_fan = !sys->actuators.exhaust_fan; break;
        case 4: sys->actuators.ventilation = !sys->actuators.ventilation; break;
        case 5: sys->actuators.alarm = !sys->actuators.alarm; break;
        default: printf("Invalid selection.\n"); break;
    }
}

void Delay(int seconds) {
    clock_t start_time = clock();
    while (clock() < start_time + (seconds * CLOCKS_PER_SEC));
}

int main(void) {
    HVAC_System hvac;
    int choice;
    int run_simulation = 1;

    HVAC_Init(&hvac);

    while (run_simulation) {
        printf("\n====================================\n");
        printf("      INDUSTRIAL HVAC SYSTEM MENU    \n");
        printf("====================================\n");
        printf("1. Start Monitoring Loop (5 Cycles)\n");
        printf("2. Switch Mode (Current: %s)\n", hvac.mode == MODE_AUTOMATIC ? "AUTO" : "MANUAL");
        printf("3. Manual Actuator Override\n");
        printf("4. Exit\n");
        printf("Select option: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }

        switch (choice) {
            case 1: {
                /* Simulates 5 cycles then returns back to menu */
                for (int cycle = 1; cycle <= 5; cycle++) {
                    HVAC_ReadSensors(&hvac.sensors);
                    HVAC_ProcessLogic(&hvac);
                    HVAC_DisplayLCD(&hvac);
                    
                    if (hvac.status == STATUS_EMERGENCY) {
                        printf("\n[ALERT] EMERGENCY SHUTDOWN TRIGGERED DUE TO SMOKE!\n");
                    }
                    
                    printf("\nRunning Cycle %d of 5... (Waiting 2 seconds)\n", cycle);
                    Delay(2);
                }
                break;
            }
            case 2:
                hvac.mode = (hvac.mode == MODE_AUTOMATIC) ? MODE_MANUAL : MODE_AUTOMATIC;
                printf("\nSwitched mode to: %s\n", hvac.mode == MODE_AUTOMATIC ? "AUTOMATIC" : "MANUAL");
                break;

            case 3:
                if (hvac.mode == MODE_MANUAL) {
                    HVAC_ManualOverride(&hvac);
                } else {
                    printf("\n[ERROR] Switch to MANUAL mode first (Option 2) to override actuators.\n");
                }
                break;

            case 4:
                run_simulation = 0;
                printf("\nExiting HVAC System Simulation. Goodbye!\n");
                break;

            default:
                printf("\nInvalid selection!\n");
                break;
        }
    }

    return 0;
}