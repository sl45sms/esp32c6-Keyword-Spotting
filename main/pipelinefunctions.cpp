
#include "pipelinefunctions.h"

static const char *TAG = "KS_PIPELINE_FUNCTIONS";

void reset_pipeline(audio_pipeline_handle_t pipeline){

    //SET LOG LEVEL
    esp_log_level_set(TAG, ESP_LOG_INFO);
    
    //cleanup
    audio_pipeline_stop(pipeline);
    //wait for the http stream to finish
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_reset_ringbuffer(pipeline);
    audio_pipeline_reset_elements(pipeline);
    audio_pipeline_terminate(pipeline);
}