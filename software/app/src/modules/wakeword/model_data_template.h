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
const unsigned char g_wakeword_model[] = {
    0x1c, 0x00, 0x00, 0x00, 0x54, 0x46, 0x4c, 0x33,
    // ... model data would continue here ...
    // This is just a minimal TFLite header signature
};

const unsigned int g_wakeword_model_len = sizeof(g_wakeword_model);

#ifdef __cplusplus
}
#endif

#endif /* WAKEWORD_MODEL_DATA_H */
