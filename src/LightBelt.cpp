#include "LightBelt.h"

LightBelt::LightBelt(uint8_t pin, uint8_t numLayers, uint8_t ledsInLayer) 
    : layers(numLayers), ledsPerLayer(ledsInLayer) {
    totalLeds = numLayers * ledsInLayer;
    strip = Adafruit_NeoPixel(totalLeds, pin, NEO_GRB + NEO_KHZ800);
}

void LightBelt::begin() {
    strip.begin();
    strip.show();
}

void LightBelt::setLayerColor(uint8_t layer, uint32_t color) {
    if (layer >= layers) return;
    
    uint16_t startLed = layer * ledsPerLayer;
    uint16_t endLed = startLed + ledsPerLayer;
    
    for (uint16_t i = startLed; i < endLed; i++) {
        strip.setPixelColor(i, color);
    }
    strip.show();
}

void LightBelt::setAllLayersColor(uint32_t color) {
    for (uint8_t i = 0; i < layers; i++) {
        setLayerColor(i, color);
    }
}

void LightBelt::rainbowCycle(uint32_t periodMs) {
    uint32_t timeNow = millis();
    uint8_t wheelPos = ((timeNow % periodMs) * 256) / periodMs;
    
    for (uint8_t layer = 0; layer < layers; layer++) {
        uint8_t adjustedWheelPos = (wheelPos + (layer * 256 / layers)) & 255;
        setLayerColor(layer, wheel(adjustedWheelPos));
    }
}

uint32_t LightBelt::wheel(byte wheelPos) {
    wheelPos = 255 - wheelPos;
    if (wheelPos < 85) {
        return strip.Color(255 - wheelPos * 3, 0, wheelPos * 3);
    }
    if (wheelPos < 170) {
        wheelPos -= 85;
        return strip.Color(0, wheelPos * 3, 255 - wheelPos * 3);
    }
    wheelPos -= 170;
    return strip.Color(wheelPos * 3, 255 - wheelPos * 3, 0);
}
