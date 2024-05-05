#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/sleep.h"
#include "hardware/rtc.h"
#include "hardware/pll.h"
#include "hardware/clocks.h"
#include "hardware/xosc.h"
#include "hardware/rosc.h"
#include "hardware/regs/io_bank0.h"
// For __wfi
#include "hardware/sync.h"
// For scb_hw so we can enable deep sleep
#include "hardware/structs/scb.h"
//https://deepbluembedded.com/raspberry-pi-pico-w-digital-inputs-outputs-c-sdk-rp2040/
//definiere Ausgaenge
const uint buttonPowerPin = 3;
const uint buttonLedPin = 2;
const uint raspiPowerPin = 28;
const uint raspiShutdownCommandPin = 8;
const uint onboardLed = 25;
//definiere Eingaenge
const uint buttonPin = 4;
const uint raspiPowerStatusPin = 9;
const uint programmingPin = 5;
//define clock values
uint scb_orig; 
uint clock0_orig;
uint clock1_orig;

void init_gpios(){
    const uint outputPins[] = {buttonPowerPin,buttonLedPin,raspiPowerPin,raspiShutdownCommandPin, onboardLed};
    const uint inputPins[] = {buttonPin, raspiPowerStatusPin, programmingPin};
    //init output gpio pins
    for (int i = 0; i < 5; i++){
        gpio_init(outputPins[i]);
        gpio_set_dir(outputPins[i], GPIO_OUT);
    }
    //init input gpio pins
    for (int i = 0; i < 3; i++){
        gpio_init(inputPins[i]);
        gpio_set_dir(inputPins[i], GPIO_IN);
        //activate pull down resistor for all inputs
        gpio_pull_down(inputPins[i]);
    }
    gpio_put(buttonPowerPin, 1);
}

void shutdown_pi(){
    //send signal to pi to shutdown
    gpio_put(raspiShutdownCommandPin, 1);
    uint timer = 0;
    //blink buttonLED until pi is shutdown
    while (gpio_get(raspiPowerStatusPin)){
        //enable led
        gpio_put(buttonLedPin, 1);
        sleep_ms(500);
        //disable led
        gpio_put(buttonLedPin, 0);
        sleep_ms(500);
        timer ++;
        //if pi not shutdown after 5 minutes (hung?)
        if (timer > 300){
            break;
        }
    }
    //wait 4 seconds until power offstart_sleep
    sleep_ms(4000);
    //set pi shutdown signal off
    gpio_put(raspiShutdownCommandPin, 0);
    //deactivate power relais, power off pi
    gpio_put(raspiPowerPin, 0);
    //disable led
    gpio_put(buttonLedPin, 0);
}

bool boot_pi(){
    //activate power relais, power pi
    gpio_put(raspiPowerPin, 1);
    uint timer = 0;
    //blink buttonLED until pi is started
    while (!gpio_get(raspiPowerStatusPin)){
        //disable led
        gpio_put(buttonLedPin, 0);
        sleep_ms(500);
        //enable led
        gpio_put(buttonLedPin, 1);
        sleep_ms(500);
        timer++;
        //if pi not started after 5 minutes (hung?)
        if (timer > 300){
            shutdown_pi();
            return false;
        }
    }
    //wait 4 seconds
    sleep_ms(4000);
    return true;
}

void start_sleep(){
    // save current values for clocks
    scb_orig = scb_hw->scr;
    clock0_orig = clocks_hw->sleep_en0;
    clock1_orig = clocks_hw->sleep_en1;
    // Wait for the fifo to be drained so we get reliable output
    uart_default_tx_wait_blocking();
    //xosc takes command while ring Oscillator sleep
    sleep_run_from_xosc();
    //sleep until buttonPin press
    sleep_goto_dormant_until_edge_high(buttonPin);
}

void recover_from_sleep(){
    //Re-enable ring Oscillator control
    rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS);
    //reset procs back to default
    scb_hw->scr = scb_orig;
    clocks_hw->sleep_en0 = clock0_orig;
    clocks_hw->sleep_en1 = clock1_orig;
    //reset clocks
    clocks_init();
    // Wait for the fifo to be drained so we get reliable output
    uart_default_tx_wait_blocking();
}

int main(){
    stdio_init_all(); // Initialisierung der Standard-Ein-/Ausgabe
    init_gpios();
    //enable onboard led
    //gpio_put(onboardLed, 1);
    //set system frequency to 30 MHz
    set_sys_clock_khz(30000, true);
    bool pi_running = false;
    while(true){
        if (!pi_running){
            //goto deepsleep until button press
            start_sleep();
            recover_from_sleep();
            if (boot_pi()){
                pi_running = true;
            }
        }else{
            //check if buttonPin press
            if (gpio_get(buttonPin)){
                shutdown_pi();
                pi_running = false;
            }else{
                //check if pi is not running (shutdown external)
                if(!gpio_get(raspiPowerStatusPin)){
                    shutdown_pi();
                    //disable led
                    gpio_put(buttonLedPin, 0);
                    pi_running = false;
                }
            }
        }
        sleep_ms(500);
    }
    return 0;
}