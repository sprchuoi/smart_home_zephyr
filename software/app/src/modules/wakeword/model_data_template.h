/*
 * Sample TensorFlow Lite Model Header
 * 
 * This is a template for embedding a TensorFlow Lite model in your firmware.
 * Replace the model_data array with your actual model converted using:
 * 
 *   xxd -i your_model.tflite > model_data.h
 * 
 * Then link this file in your build system.
 */

#ifndef WAKEWORD_MODEL_DATA_H
#define WAKEWORD_MODEL_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

// Sample model data (this is just a placeholder - replace with your actual model)
// To generate real model data:
// 1. Train your model using TensorFlow/Keras
// 2. Convert to TFLite: converter = tf.lite.TFLiteConverter.from_keras_model(model)
// 3. Convert to C array: xxd -i model.tflite > model_data.h
// 4. Copy the array data here

// Placeholder model (not functional - for compilation only)
// WARNING: This is NOT a valid TensorFlow Lite model!
// This template is provided as a reference for the data structure only.
// Do NOT use CONFIG_APP_WAKEWORD_MODEL_PATH="g_wakeword_model" unless you replace
// this array with a real TFLite model converted using:
//   xxd -i your_model.tflite > model_data.h
const unsigned char g_wakeword_model[] = {
    0x1c, 0x00, 0x00, 0x00, 0x54, 0x46, 0x4c, 0x33,
    // ... model data would continue here ...
    // This is just a minimal TFLite header signature
};

const unsigned int g_wakeword_model_len = sizeof(g_wakeword_model);

// Compile-time check to prevent accidental use of template
#if defined(CONFIG_APP_WAKEWORD_TFLITE) && \
    !defined(ALLOW_TEMPLATE_MODEL_DATA)
#warning "Using model_data_template.h with TFLite backend. Replace with real model!"
#endif

#ifdef __cplusplus
}
#endif

#endif /* WAKEWORD_MODEL_DATA_H */
