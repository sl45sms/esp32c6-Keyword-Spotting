#include <string.h>
#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"
#include "board.h"
#include "esp_log.h"
#include "esp_err.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "i2s_stream.h"
#include "raw_stream.h"
#include "esp_timer.h"

//gpio
#include "driver/gpio.h"
//led
#include "light.h"
//pipelinefunctions
#include "pipelinefunctions.h"
//main
#include "main.h"
//ei
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

//#define EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW 12

//edge impulse settings sould be the same as in the i2s mic
const int EI_CLASSIFIER_AUDIO_SAMPLE_RATE = MIC_AUDIO_SAMPLE_RATE;
const int EI_CLASSIFIER_AUDIO_SAMPLE_BITS = MIC_AUDIO_BITS;
const int EI_CLASSIFIER_AUDIO_SAMPLE_CHANNELS = MIC_AUDIO_CHANNELS;

//alocate memory for the raw buffer
int16_t *raw_input_buf =(int16_t *)malloc(EI_CLASSIFIER_SLICE_SIZE*sizeof(short));

bool wake_word_detected = false;

static const char *TAG = "REC_RAW_HTTP";

audio_pipeline_handle_t edge_impulse_pipeline,
                        mic_pipeline;
audio_element_handle_t mic_i2s_stream_reader,
                       mic_raw_stream_reader,
                       mic_http_stream_writer;

void audio_recorder_maxtime_cb(void *arg)
{
    reset_pipeline(mic_pipeline);
}

//EI Callback: fill a section of the out_ptr buffer when requested
static int get_and_convert_mic_data(size_t offset, size_t length, float *out_ptr) {
 //Read raw audio data from the buffer and convert to float
    return numpy::int16_to_float(raw_input_buf + offset, out_ptr, length);
}


void start_ei_pipeline(){
    ESP_LOGI(TAG, "[ 1 ] - EI Register all elements to audio pipeline");
    audio_pipeline_register(edge_impulse_pipeline, mic_i2s_stream_reader, "ei_mic_i2s");
    audio_pipeline_register(edge_impulse_pipeline, mic_raw_stream_reader, "ei_mic_raw");
    const char *ei_link_tag[2] = {"ei_mic_i2s", "ei_mic_raw"};
    ESP_ERROR_CHECK(audio_pipeline_link(edge_impulse_pipeline, &ei_link_tag[0], 2));
    ESP_LOGI(TAG, "[ 2 ]- EI Run audio pipeline");
    audio_pipeline_run(edge_impulse_pipeline);
}

void detect_wake_word_start()
{
    signal_t signal;            // Wrapper for raw input buffer
    ei_impulse_result_t result; // Used to store inference output
    EI_IMPULSE_ERROR res;       // Return code from inference

    // Assign callback function to fill buffer used for preprocessing/inference
    signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
    signal.get_data = &get_and_convert_mic_data;

ESP_LOGI(TAG, "[ 3 ] - EI slice per model %d", EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);
//reset/empty raw_input_buf
memset(raw_input_buf, 0, EI_CLASSIFIER_SLICE_SIZE * sizeof(short));
run_classifier_init();//to prevent remaining data from previous runs

start_ei_pipeline();//start the pipeline
int detect=0;
light(false,false);//turn off the led
while (1)
{

   // Read raw audio data from the buffer this takes 0.25 second, the full buffer filled at 1 second
   int64_t br = raw_stream_read(mic_raw_stream_reader, (char *)raw_input_buf, EI_CLASSIFIER_SLICE_SIZE * sizeof(short));
   // Perform DSP pre-processing and inference so have to run this every 0.25 seconds
   res = run_classifier_continuous(&signal, &result, false);
   if (res != EI_IMPULSE_OK) {
        printf("ERR: Failed to run classifier (%d)\r\n", res);
        continue;
    }
    if (result.classification[2].value > 0.9 || gpio_get_level(GPIO_NUM_9) == 0 ) { //Vrasida detected
        //may save here the buffer to a file to check for false positives
        detect++;
        printf("Detected %s (%f %f %f)\r\n", ei_classifier_inferencing_categories[2], result.classification[2].value,result.classification[1].value,result.classification[0].value);
      
      if (detect > 0 || gpio_get_level(GPIO_NUM_9) == 0 ) //wait for 2 consecutive detections
      {
       if (gpio_get_level(GPIO_NUM_9) != 0 ) wake_word_detected = true;
       //reset and unregister the edge impulse pipeline
       reset_pipeline(edge_impulse_pipeline);
       audio_pipeline_unregister(edge_impulse_pipeline, mic_i2s_stream_reader);
       audio_pipeline_unregister(edge_impulse_pipeline, mic_raw_stream_reader);
      //break the loop
      break;
     }

    } else {
       detect=0;
       printf("Not detected %s (%f)\r", ei_classifier_inferencing_categories[2], result.classification[2].value);
    }
    vTaskDelay(10/ portTICK_PERIOD_MS); //to prevent the task timeout
}
      ESP_LOGI(TAG, "[ 4 ] - EI Stop audio pipeline");
}


//use extern c to be able to complile c++ code
extern "C" void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_ERROR);
    esp_log_level_set(TAG, ESP_LOG_INFO);
    
    light(false,true);//init the led at gpio8 to indicate recording

////////////////PIPELINES//////////////////////////

    ESP_LOGI(TAG, "[ 5 ] EI Create audio pipeline for wake word detection");
    audio_pipeline_cfg_t edge_impulse_pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    edge_impulse_pipeline = audio_pipeline_init(&edge_impulse_pipeline_cfg);
    mem_assert(edge_impulse_pipeline);

    /* connect mic pins as is on board_pins_config.h
    i2s_config->bck_io_num = GPIO_NUM_22; // BCKL
    i2s_config->ws_io_num = GPIO_NUM_21; // WS
    i2s_config->data_in_num = GPIO_NUM_15; // DOUT
    */
    ESP_LOGI(TAG, "[3.1] - MIC Create i2s stream to read audio data from i2s mic");
    i2s_stream_cfg_t mic_i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    mic_i2s_cfg.type = AUDIO_STREAM_READER;
    mic_i2s_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    mic_i2s_cfg.use_alc=true;
    mic_i2s_cfg.volume = 20; //max is 64
    //mic_i2s_cfg.out_rb_size = 16 * 4096; // Increase buffer to avoid missing data in bad network conditions
    mic_i2s_stream_reader = i2s_stream_init(&mic_i2s_cfg);
    ESP_ERROR_CHECK(i2s_stream_set_clk(mic_i2s_stream_reader, EI_CLASSIFIER_AUDIO_SAMPLE_RATE, EI_CLASSIFIER_AUDIO_SAMPLE_BITS, EI_CLASSIFIER_AUDIO_SAMPLE_CHANNELS));

    ESP_LOGI(TAG, "[ 6 ]- EI create raw stream to write data to edge impulse buffer");
    raw_stream_cfg_t raw_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_cfg.type = AUDIO_STREAM_READER;
    mic_raw_stream_reader = raw_stream_init(&raw_cfg);

//main loop
int16_t count = 0;
while (1) {
    ESP_LOGI(TAG, "[ 7 ] - Detected Count %d", count++);
    detect_wake_word_start();
    light(true,false); //turn on the led
    //delay for 1 second to
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}//loop end

}
