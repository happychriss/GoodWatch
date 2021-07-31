//
// Created by development on 22.01.21.
//

// clang-format on
#include "global.h"
#include "support.h"
#include "driver/i2s.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/task.h"
#include "edge-impulse-sdk/dsp/numpy.hpp"
#include "inference_parameter.h"
#include "edgeimpulse/ff_command_set_inference.h"



/** Audio buffers, pointers and selectors */
typedef struct {
    signed short *buffers[2];
    unsigned char buf_select;
    unsigned char buf_ready;
    unsigned int buf_count;
    unsigned int n_samples;
} inference_t;

extern inference_t inference;

//extern inference_t inference;
extern bool record_ready;
extern signed short *sampleBuffer;
extern bool debug_nn; // Set this to true to see e.g. features generated from the raw signal

extern QueueHandle_t m_i2sQueue;

/**
 * @brief      Init inferencing struct and setup/start PDM
 *
 * @param[in]  n_samples  The n samples
 *
 * @return     { description_of_the_return_value }
 */


void ei_print_config() {// summary of inferencing settings (from model_metadata.h)
    ei_printf("Inferencing settings:\n");
    ei_printf("\tInterval: %.2f ms.\n", (float) EI_CLASSIFIER_INTERVAL_MS);
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\tSlices per model window %d\n", EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);
    ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));
}


void i2s_init(void) {
    // Start listening for audio: MONO @ 16KHz

    i2s_pin_config_t pin_config_micro = {
            .bck_io_num = I2S_NUM_2_BCLK, //this is BCK pin
            .ws_io_num = I2S_NUM_2_LRC, // this is LRCK pin
            .data_out_num = I2S_PIN_NO_CHANGE, // this is DATA output pin
            .data_in_num = I2S_NUM_2_DIN   //DATA IN
    };

    // https://github.com/atomic14/ICS-43434-breakout-board
    i2s_config_t i2s_config_micro = {
            .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_RX),
            .sample_rate = 16000,

            //  The data word size is set at 32 bits and "I2S_PHILIPS_MODE" format. So a single stereo frame consists of two 32-bit PCM words or 8 bytes.
            //  The actual PCM audio data is 24 bits wide, is signed and is stored in little-endian format with 8 bits of left-justified 0 "padding".
            .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,

            .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // was I2S_CHANNEL_FMT_RIGHT_LEFT
            .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S),
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count = 4,
            .dma_buf_len = 1024,
            .use_apll = false,
            .tx_desc_auto_clear = false,
            .fixed_mclk = 0
    };


    esp_err_t ret = 0;
    ret = i2s_driver_install((i2s_port_t) I2S_MICRO, &i2s_config_micro, sizeof(i2s_event_t), &m_i2sQueue);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error in i2s_driver_install");
    }
    ret = i2s_set_pin((i2s_port_t) I2S_MICRO, &pin_config_micro);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error in i2s_set_pin");
    }

    ret = i2s_zero_dma_buffer((i2s_port_t) I2S_MICRO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error in initializing dma buffer with 0");
    }

}

void addSample(int16_t sample) {
    inference.buffers[inference.buf_select][inference.buf_count++] = sample;

    if (inference.buf_count >= inference.n_samples) {
        inference.buf_select ^= 1;
        inference.buf_count = 0;
        inference.buf_ready = 1;
    }
}

// https://github.com/atomic14/ICS-43434-breakout-board/tree/main/sketch
void CaptureSamples(void *arg) {

    i2s_init();
    DP("CaptureSamples - Running on Core:");
    DPL(xPortGetCoreID());

    while (true) {
        // wait for some data to arrive on the queue
        i2s_event_t evt;
        if (xQueueReceive(m_i2sQueue, &evt, portMAX_DELAY) == pdPASS) {
            if (evt.type == I2S_EVENT_RX_DONE) {
                {
                    size_t bytes_read = 0;
                    // read data from the I2S peripheral
                    do {

                        uint8_t i2s_data[1024];
                        // read from i2s

                        i2s_read(
                                (i2s_port_t) I2S_MICRO,
                                i2s_data,
                                1024,
                                &bytes_read,
                                10);

                        // process the raw data
                        int32_t *samples = (int32_t *) i2s_data;
                        for (int i = 0; i < bytes_read / 4; i++) {
                            // you may need to vary the >> 11 to fit your volume - ideally we'd have some kind of AGC here
                            // add sample loads a 16bit value in this implementation

                            addSample(samples[i] >> 11);
                        }
                    } while (bytes_read > 0);
                }
            }
        }

    }

}


bool microphone_inference_start(uint32_t n_samples) {
    inference.buffers[0] = (signed short *) malloc(n_samples * sizeof(signed short));

    if (inference.buffers[0] == NULL) {
        return false;
    }

    inference.buffers[1] = (signed short *) malloc(n_samples * sizeof(signed short));

    if (inference.buffers[0] == NULL) {
        free(inference.buffers[0]);
        return false;
    }

    // Sample buffer has half of the size of inference buffer (cn)
    sampleBuffer = (signed short *) malloc((n_samples >> 1) * sizeof(signed short));

    if (sampleBuffer == NULL) {
        free(inference.buffers[0]);
        free(inference.buffers[1]);
        return false;
    }

    inference.buf_select = 0;
    inference.buf_count = 0;
    inference.n_samples = n_samples;
    inference.buf_ready = 0;

    record_ready = true;

    xTaskCreatePinnedToCore(CaptureSamples, "CaptureSamples", 1024 * 32, NULL, 1, NULL, 0);

    return true;
}

/**
 * @brief      Wait on new data
 *
 * @return     True when finished
 */


bool microphone_inference_record(void) {
    bool ret = true;

    if (inference.buf_ready == 1) {
//  todo: uncomment error message
        //        DP("Error sample buffer overrun. Decrease the number of slices per model window, (EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)\n");
        DPL("-");
        ret = false;
    }

    while (inference.buf_ready == 0) {
        delay(1);
    }

    inference.buf_ready = 0;
    return ret;
}

/**
 * @brief      Stop PDM and release buffers
 */


void microphone_inference_end(void) {
//    PDM.end();
    free(inference.buffers[0]);
    free(inference.buffers[1]);
    free(sampleBuffer);
}

/**
 * @brief      PDM buffer full callback
 *             Get data and call audio thread callback
 */


int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
    ei::numpy::int16_to_float(&inference.buffers[inference.buf_select ^ 1][offset], out_ptr, length);

    return 0;
}


#define STATE_NOTHING 0
#define STATE_FOUND 1
#define STATE_FOUND_MISSED_ONE 2
#define STATE_FOUND_MISSED_TWO 3
#define STATE_RESOLVED 4


int inference_get_category_idx() {
// Inference Global Data

    int print_results = 0;
    short result_state = STATE_NOTHING;
    float values[EI_CLASSIFIER_LABEL_COUNT] = {0.0};

    while (true) {

        bool m = microphone_inference_record();

        if (!m) {
            // todo uncomment error message
            // ei_printf("ERR: Failed to record audio...\n");
            return INF_ERROR;
        }

        signal_t signal;
        signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
        signal.get_data = &microphone_audio_signal_get_data;
        ei_impulse_result_t result = {0};

        EI_IMPULSE_ERROR r = run_classifier_continuous(&signal, &result, debug_nn);
        if (r != EI_IMPULSE_OK) {
            ei_printf("ERR: Failed to run classifier (%d)\n", r);
            return INF_ERROR;
        }

        if (++print_results >= (0)) {
//print the predictions
//       ei_printf("Predictions "); ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)", result.timing.dsp, result.timing.classification, result.timing.anomaly);
//           ei_printf(": \n");
            bool b_found_result = false;
            int max_value_idx = -1;
            float max_value = 0;

            // Find the best value, and note if more then 85%
            for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
                if ((result.classification[ix].value > 0.85) && (result.classification[ix].label[0] != 'N')) {
//                ei_printf("\nResults:\n");
                    b_found_result = true;
                    if (result.classification[ix].value > max_value) {
                        max_value = result.classification[ix].value;
                        max_value_idx = ix;
                    }
                }
            }

            // Print list of result values
            if (b_found_result) {
                for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
                    ei_printf("    %s: %.5f", result.classification[ix].label, result.classification[ix].value);
                    if (max_value_idx == ix) ei_printf("<-----\n"); else ei_printf("\n");
                }
                ei_printf("\n");
            } //else { ei_printf("."); }

            if (b_found_result) {
                result_state = STATE_FOUND;
                values[max_value_idx] = values[max_value_idx] + max_value;
            } else {
                switch (result_state) {
                    case STATE_NOTHING:
                        result_state = STATE_NOTHING;
                        break;
                    case STATE_FOUND:
                        result_state = STATE_FOUND_MISSED_ONE;
                        break;
                    case STATE_FOUND_MISSED_ONE:
                        result_state = STATE_FOUND_MISSED_TWO;
                        break;
                    case STATE_FOUND_MISSED_TWO:
                        result_state = STATE_RESOLVED;
                        break;
                    default:
                        DPL("aaaaaaaaaaaaaaaaargh");
                        DPL(result_state);
                }
            }

            if (result_state == STATE_RESOLVED) {
                int final_value_idx = -1;
                float final_value = 0.0;

                for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
                    if (values[ix] > final_value) {
                        final_value = values[ix];
                        final_value_idx = ix;
                    }
                }
                DP("\n------------> ");
//                DP(ei_classifier_inferencing_categories[final_value_idx]);
                DP(" - ");
                DPL(final_value);
                return final_value_idx;

            }
//            ei_printf("***********************    %s: %.5f", ei_classifier_inferencing_categories[final_value_idx], my_final_value);
//            memset(values, 0, sizeof(values));

        }
    }
}