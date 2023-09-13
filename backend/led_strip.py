#!/usr/bin/env python3
# NeoPixel library strandtest example
# Author: Tony DiCola (tony@tonydicola.com)
#
# Direct port of the Arduino NeoPixel library strandtest example.  Showcases
# various animations on a strip of NeoPixels.

from threading import Thread
import RPi.GPIO as GPIO
import time
from rpi_ws281x import PixelStrip, Color
import argparse
from enum import Enum
# LED strip configuration:
LED_COUNT = 54        # Number of LED pixels.
# LED_PIN = 18          # GPIO pin connected to the pixels (18 uses PWM!).
LED_PIN = 12        # GPIO pin connected to the pixels (10 uses SPI /dev/spidev0.0).
LED_FREQ_HZ = 800000  # LED signal frequency in hertz (usually 800khz)
LED_DMA = 10          # DMA channel to use for generating signal (try 10)
LED_BRIGHTNESS = 255  # Set to 0 for darkest and 255 for brightest
LED_INVERT = False    # True to invert the signal (when using NPN transistor level shift)
LED_CHANNEL = 0       # set to '1' for GPIOs 13, 19, 41, 45 or 53

GPIO.setwarnings(False)

led_relay = 25
GPIO.setmode(GPIO.BCM)
GPIO.setup(led_relay, GPIO.OUT)

LedMethod = Enum('LedMethod', ['OFF', 'COLOR_WIPE', 'THEATRE_CHASE', 'RAINBOW', "RAINBOW_CYCLE", "RAINBOW_THEATRE_CHASE"])

class LedStripThread(Thread):

    def __init__(self):
        super().__init__()
        self.__method = LedMethod.OFF
        self.__color = Color(0, 0, 0)
        self.__strip = PixelStrip(LED_COUNT, LED_PIN, LED_FREQ_HZ, LED_DMA, LED_INVERT, LED_BRIGHTNESS, LED_CHANNEL)
        self.__strip.begin()
        self.running = True

    def run(self):
        print(".")
        while self.running:
            if(self.__method == LedMethod.OFF):
                self.__off()
            elif(self.__method == LedMethod.COLOR_WIPE):
                self.__colorWipe()
            elif(self.__method == LedMethod.THEATRE_CHASE):
                self.__theaterChase()
            elif(self.__method == LedMethod.RAINBOW):
                self.__rainbow()
            elif(self.__method == LedMethod.RAINBOW_CYCLE):
                self.__rainbowCycle()
            elif(self.__method == LedMethod.RAINBOW_THEATRE_CHASE):
                self.__theaterChaseRainbow()
        print("LEDStripThread done")
    
    def stop(self):
        print("LEDStripThread stopping")
        self.__method = None
        self.running = False

    def hex_to_rgb(self, hex):
        rgb = []
        for i in (0, 2, 4):
            decimal = int(hex[i:i+2], 16)
            rgb.append(decimal)
        
        return Color(rgb[0], rgb[1], rgb[2])

    def rgb_to_hex(self, color:Color):
        return '#' + f'{color.r:02}'  + f'{color.g:02}'  + f'{color.b:02}'

    def cfg(self, method:LedMethod, hex_color:str = "#000000"):
        self.__method = method
        self.__color = self.hex_to_rgb(hex_color)
    
    def set_method(self, method:LedMethod):
        self.__method = method

    def set_color(self, hex_color):
        self.__color = self.hex_to_rgb(hex_color)

    def set_brightness(self, value):
        self.__strip.setBrightness(value)

    def get_cfg(self):
        return {"color" : self.rgb_to_hex(self.__color),
                "method" : self.__method.name,
                "brightness" : self.__strip.getBrightness()}
    
    def __off(self):
        self.__color = Color(0, 0, 0)
        self.__colorWipe(10) 
        time.sleep(1)

    # Define functions which animate LEDs in various ways.
    def __colorWipe(self, wait_ms=50):
        
        """Wipe color across display a pixel at a time."""
        for i in range(self.__strip.numPixels()):
            self.__strip.setPixelColor(i, self.__color)
            self.__strip.show()
            time.sleep(wait_ms / 1000.0)
        time.sleep(1)

    def __theaterChase(self, wait_ms=50, iterations=10):
        """Movie theater light style chaser animation."""
        for j in range(iterations):
            if not self.__method == LedMethod.THEATRE_CHASE: break
            for q in range(3):
                if not self.__method == LedMethod.THEATRE_CHASE: break
                for i in range(0, self.__strip.numPixels(), 3):
                    if not self.__method == LedMethod.THEATRE_CHASE: break
                    self.__strip.setPixelColor(i + q, self.__color)
                self.__strip.show()
                time.sleep(wait_ms / 1000.0)
                for i in range(0, self.__strip.numPixels(), 3):
                    if not self.__method == LedMethod.THEATRE_CHASE: break
                    self.__strip.setPixelColor(i + q, 0)

    def __wheel(self, pos):
        """Generate rainbow colors across 0-255 positions."""
        if not self.running: return
        if pos < 85:
            return Color(pos * 3, 255 - pos * 3, 0)
        elif pos < 170:
            pos -= 85
            return Color(255 - pos * 3, 0, pos * 3)
        else:
            pos -= 170
            return Color(0, pos * 3, 255 - pos * 3)

    def __rainbow(self, wait_ms=200, iterations=1):
        """Draw rainbow that fades across all pixels at once."""
        for j in range(256 * iterations):
            if not self.__method == LedMethod.RAINBOW: break
            for i in range(self.__strip.numPixels()):
                if not self.__method == LedMethod.RAINBOW: break
                self.__strip.setPixelColor(i, self.__wheel((i + j) & 255))
            self.__strip.show()
            time.sleep(wait_ms / 1000.0)

    def __rainbowCycle(self, wait_ms=20, iterations=5):
        """Draw rainbow that uniformly distributes itself across all pixels."""
        for j in range(256 * iterations):
            if not self.__method == LedMethod.RAINBOW_CYCLE: break
            for i in range(self.__strip.numPixels()):
                if not self.__method == LedMethod.RAINBOW_CYCLE: break
                self.__strip.setPixelColor(i, self.__wheel(
                    (int(i * 256 / self.__strip.numPixels()) + j) & 255))
            self.__strip.show()
            time.sleep(wait_ms / 1000.0)

    def __theaterChaseRainbow(self, wait_ms=50):
        """Rainbow movie theater light style chaser animation."""
        for j in range(256):
            for q in range(3):
                if not self.__method == LedMethod.RAINBOW_THEATRE_CHASE: break
                for i in range(0, self.__strip.numPixels(), 3):
                    if not self.__method == LedMethod.RAINBOW_THEATRE_CHASE: break
                    self.__strip.setPixelColor(i + q, self.__wheel((i + j) % 255))
                self.__strip.show()
                time.sleep(wait_ms / 1000.0)
                for i in range(0, self.__strip.numPixels(), 3):
                    if not self.__method == LedMethod.RAINBOW_THEATRE_CHASE: break
                    self.__strip.setPixelColor(i + q, 0)