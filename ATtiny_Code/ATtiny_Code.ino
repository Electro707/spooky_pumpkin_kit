/************************************************************************************************************************
*  Touch Pumpkin Attiny85 Code
*  By Jamal Bouajjaj
*
*  This program controls the LEDs on the 2019 UNH Makerspace Touch Pumpkin soldering kit. This program will wait for
*    a capacitive touch to reach above a threshold, in which case it will switch the LED mode to fading, blinking, and
*    off sequentially.
************************************************************************************************************************/
#include <CapacitiveSensor.h>     // Import the library
#include <avr/sleep.h>
#include<avr/wdt.h>
#define EYE_LED_PIN PB0           // Define the eye control LED pin
#define EYE_LEFT_LEN_PIN PB2      // Define the other eye control light if the jumper is cut away
#define MOUTH_LED_PIN PB1         // Define the LED pin
#define CAPSENSE_SEND PB4         // Define the capacitive touch send pin
#define CAPSENSE_REC  PB3         // Define the capacitive touch recieve pin

typedef uint8_t flag;

int mode = 0;                     // Store the current LED mode
bool last_state = false;          // Store the last capacitive touch state ( pressed or not pressed)

int pwm_val = 0;                  // Store the PWM value during fading mode
bool pwm_direction = false;       // Store the directions of the changing PWM during fading mode ( fade up or down)

int old_millis = 0;               // Store the previous time (millis()) during blinking and fading mode
int old_millis2 = 0;               // Store the previous time (millis()) during blinking and fading mode
int current_time = 0;             // Store the current time (millis()) for comparision with old_millis

uint8_t mode_fast_eye_blink = 0;
flag mode_fast_eye_increment = false;
uint8_t mode_randon_eye_blink_state = 0;
uint8_t mode_randon_eye_blink_pwm = 0;
uint8_t mode_randon_eye_blink_i = 0;

CapacitiveSensor cs = CapacitiveSensor(CAPSENSE_SEND, CAPSENSE_REC);   // Create the CapacitiveSensor class

enum OperatingModes{
OP_MODE_SLEEP = 0,
OP_MODE_FADE_ALL,
OP_MODE_RANDOM_EYE_BLINK,
OP_MODE_FADE_FAST_EYE,
OP_MODE_BLINK_ALL,
OP_LAST_MODE    // Not actually a mode, just here to
};

void setup()                    
{
    wdt_off();
    cs.set_CS_AutocaL_Millis(0xFFFFFFFF);     // Turn off autocalibrate on the capacitive touch channel
    pinMode(MOUTH_LED_PIN, OUTPUT);                      // Set the LED pin as an output
    pinMode(EYE_LED_PIN, OUTPUT);                      // Set the LED pin as an output
    // The following code is to save power on unused preiferals
    ACSR |= (1<<ACD);              // Turn off Analog Comparator
    ADCSRA &= ~(1<<ADEN);           // Turn off ADC
    PRR |= (1<<PRADC | 1<<PRUSI);             // Shut down ADC
}

void loop()                    
{
    long cs_val =  cs.capacitiveSensor(30);     // Detect the arbritrary capacitance on the copper pad

    // If above threshold and previous state is off, increment the operating mode
    if(cs_val > 110){
        if(last_state == false){    // If pressed and the previous state is off, change the mode
            last_state = true;
            pwm_val = 0;
            pwm_direction = 0;
            mode++;
            if(mode >= OP_LAST_MODE){
                mode = 0;
            }
            // LED Initialization for blinking modes
            switch(mode){
                case OP_MODE_BLINK_ALL:
                    digitalWrite(MOUTH_LED_PIN, HIGH);
                    digitalWrite(EYE_LED_PIN, LOW);
                    break;
                case OP_MODE_FADE_ALL:
                    digitalWrite(MOUTH_LED_PIN, LOW);
                    digitalWrite(EYE_LED_PIN, LOW);
                    break;
                default:
                    digitalWrite(MOUTH_LED_PIN, LOW);
                    digitalWrite(EYE_LED_PIN, HIGH);
                    break;
            }
            old_millis = millis();
            old_millis2 = old_millis;
            delay(150);
        }
    }
    else{   // If the previous press state is on and the pad isn't pressed, revert the previous pressed state to false
        if(last_state == true){
            last_state = false;
            delay(50);
        }
    }

    // Drive the LEDs based on the current operating mode
    switch(mode){
        case OP_MODE_SLEEP:       // Mode 0 <- Off, go to sleep
            pinMode(MOUTH_LED_PIN, INPUT);
            pinMode(EYE_LED_PIN, INPUT);
            cli();
            wdt_reset();                    // Reset the WDT
            MCUSR &= ~(1<<WDRF);            // clear reset flag
            WDTCR = (1<<WDE) | (1<<WDCE);   // enable watchdog
            WDTCR = (1<<WDIE) | 0b110;      // watchdog interrupt instead of reset for 1 second
            set_sleep_mode(SLEEP_MODE_PWR_DOWN);    // Set the right sleep mode
            // wdt_reset();
            // Jump into sleep mode
            sleep_enable();
            sleep_bod_disable();
            sei();
            sleep_cpu();
            // Most-wake instructions, disable sleep
            sleep_disable();
            wdt_off();                          // Turn off the WDT
            // Set the IOs to the right output type
            digitalWrite(EYE_LED_PIN, HIGH);
            digitalWrite(MOUTH_LED_PIN, LOW);
            pinMode(MOUTH_LED_PIN, OUTPUT);
            pinMode(EYE_LED_PIN, OUTPUT);
            break;

        case OP_MODE_FADE_ALL:       // Mode 1 <- Fading
            current_time = millis();
            if(current_time - old_millis >= 20){
                old_millis = current_time;
                fade_mouth();
            }
            break;

        case OP_MODE_BLINK_ALL:     // Mode 2 <- Blinking
            current_time = millis();
            if(current_time - old_millis >= 1000){
                digitalWrite(MOUTH_LED_PIN, !digitalRead(MOUTH_LED_PIN));
                digitalWrite(EYE_LED_PIN, !digitalRead(EYE_LED_PIN));
                old_millis = current_time;
            }
            break;

    //      case OP_MODE_RANDOM_EYE_BLINK:
    //        current_time = millis();
    //        if(current_time - old_millis >= 100){
    //          long r = random(0, 50);
    //          if(r == 13){
    //            digitalWrite(MOUTH_LED_PIN, LOW);
    //            for(int i=255;i>=0;i-=5){
    //              analogWrite(EYE_LED_PIN, i);
    //              delay(75);
    //            }
    //            delay(1500);
    //            digitalWrite(EYE_LED_PIN, HIGH);
    //            for(int i=0;i<10;i++){
    //              digitalWrite(EYE_LED_PIN, !digitalRead(EYE_LED_PIN));
    //              digitalWrite(MOUTH_LED_PIN, !digitalRead(MOUTH_LED_PIN));
    //              delay(50);
    //            }
    //            digitalWrite(EYE_LED_PIN, HIGH);
    //            digitalWrite(MOUTH_LED_PIN, LOW);
    //          }
    //          old_millis = millis();
    //        }
    //        break;

        case OP_MODE_RANDOM_EYE_BLINK:
            current_time = millis();
            if(mode_randon_eye_blink_state == 0){
                if(current_time - old_millis >= 100){
                    long r = random(0, 50);
                    if(r == 13){
                        mode_randon_eye_blink_state = 1;
                        old_millis2 = current_time;
                        mode_randon_eye_blink_pwm = 255;
                        digitalWrite(MOUTH_LED_PIN, LOW);
                        digitalWrite(EYE_LED_PIN, HIGH);
                    }
                    old_millis = current_time;
                }
            }
            else{
                switch(mode_randon_eye_blink_state){
                    case 1:
                        if(current_time - old_millis2 >= 75){
                            old_millis2 = current_time;
                            analogWrite(EYE_LED_PIN, mode_randon_eye_blink_pwm);
                            mode_randon_eye_blink_pwm -= 5;
                            if(mode_randon_eye_blink_pwm == 0){
                                mode_randon_eye_blink_state = 2;
                            }
                        }
                        break;
                    case 2:
                        if(current_time - old_millis2 >= 1500){
                            old_millis2 = current_time;
                            mode_randon_eye_blink_state = 3;
                            mode_randon_eye_blink_i = 0;
                            digitalWrite(EYE_LED_PIN, HIGH);
                        }
                        break;
                    case 3:
                        if(current_time - old_millis2 >= 50){
                            old_millis2 = current_time;
                            digitalWrite(EYE_LED_PIN, !digitalRead(EYE_LED_PIN));
                            digitalWrite(MOUTH_LED_PIN, !digitalRead(MOUTH_LED_PIN));
                            if(++mode_randon_eye_blink_i == 10){
                                digitalWrite(EYE_LED_PIN, HIGH);
                                digitalWrite(MOUTH_LED_PIN, LOW);
                                mode_randon_eye_blink_state = 0;
                            }
                        }
                        break;
            }
            }
            break;

        case OP_MODE_FADE_FAST_EYE:
            current_time = millis();
            if(current_time - old_millis >= 20){
                old_millis = current_time;
                fade_mouth();
                long r = random(0, 66);
                if(r == 13){
                    mode_fast_eye_blink = 6;
                }
            }
            if(current_time - old_millis2 >= 50){
                old_millis2 = current_time;
                if(mode_fast_eye_blink != 0){
                    digitalWrite(EYE_LED_PIN, !digitalRead(EYE_LED_PIN));
                    mode_fast_eye_blink--;
                }
            }
            break;
    }
}

EMPTY_INTERRUPT(WDT_vect)

void fade_mouth(){
if(pwm_direction){
    if(pwm_val == 0){
    pwm_direction = false;
    }
    else{
    pwm_val--;
    }
}
else{
    if(pwm_val >= 100){
    pwm_direction = true;
    }
    else{
    pwm_val++;
    }
}
analogWrite(MOUTH_LED_PIN, pwm_val);
}

void wdt_off(void){
wdt_reset();
MCUSR = 0;
WDTCR |= (1<<WDCE) | (1<<WDE);
WDTCR = 0x00;
}
