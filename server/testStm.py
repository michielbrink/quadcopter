#! /usr/bin/sudo /usr/bin/python2
import stm
import time

with stm.stm_device() as dev:
    for i in range(20):
        dev.set_leds(0x00)
        time.sleep(1)
        dev.set_leds(0x00)
        time.sleep(1)

