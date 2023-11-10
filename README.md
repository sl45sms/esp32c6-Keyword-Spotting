# esp32c6-Keyword-Spotting
It is a project to use Edge Impulse to do keyword spotting on ESP32-C6-DevKitC-1-N8 dev board.
Just as a reference for me in the future or anyone who is interested in this topic.

# microphone
sen0526 from DFRobot, but any i2s mic with MSM261S4030H0R chip should work,
connect the mic to the board as below:
```
    i2s_config->bck_io_num = GPIO_NUM_22; // BCKL
    i2s_config->ws_io_num = GPIO_NUM_21; // WS
    i2s_config->data_in_num = GPIO_NUM_15; // DOUT
```

# install ESP-IDF and ESP-ADF
Asume you have already installed ESP-IDF(5.1) and latest ESP-ADF, if not, please refer to the official tutorials.
I've recoment the VSCode extension: https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension

# Create a new project on Edge Impulse
Just go to edgeimpulse.com and create a new project.. 
You can also refer to the official tutorial: https://docs.edgeimpulse.com/docs/tutorials/end-to-end-tutorials/responding-to-your-voice

## Setup Edge Impulse
Build and download the Edge Impulse C++ library, then extract `edge-impulse-sdk`,`model-parameters` and `tflite-model` to the root of the project.
Make a change to `edge-inpulse-sdk/porting/ei_classifier_porting.h` to include
`CONFIG_IDF_TARGET_ESP32C6` in the list of supported targets of EI_PORTING_ESPRESSIF
```c++
#ifndef EI_PORTING_ESPRESSIF
#if (defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32S3))
#define EI_PORTING_ESPRESSIF      1
#define EI_PORTING_ARDUINO        0
#else
#define EI_PORTING_ESPRESSIF     0
#endif
#endif
```

# how to use
just say the word "vrasida" and the LED will light up. (yes it is beter to train a model with easier word, but I am too lazy to do that,use your own model if you want to do that)


# some notes 
be sure you have checked the CONFIG_FREERTOS_ENABLE_BACKWARD_COMPATIBILITY=y in sdkconfig or else ADF will not be compiled.

