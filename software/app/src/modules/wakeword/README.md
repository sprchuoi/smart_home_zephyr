# Wake Word Detection Module

This module provides wake-word detection capabilities with support for multiple machine learning backends.

## Supported Backends

### 1. Edge Impulse SDK (Recommended)

Edge Impulse provides an end-to-end platform for embedded machine learning.

#### Prerequisites
- Edge Impulse account
- Trained wake-word detection model from Edge Impulse Studio

#### Integration Steps

1. **Train Your Model in Edge Impulse Studio**
   - Go to https://studio.edgeimpulse.com/
   - Create a new project for audio classification
   - Upload wake-word samples
   - Train your model
   - Download the model as "Zephyr library"

2. **Add Edge Impulse SDK to Your Project**
   
   Extract the downloaded ZIP and add to `software/lib/edge-impulse/`:
   ```
   software/lib/edge-impulse/
   ├── edge-impulse-sdk/
   ├── model-parameters/
   ├── tflite-model/
   └── edge_impulse.h
   ```

3. **Update CMakeLists.txt**
   
   Add to `software/app/CMakeLists.txt`:
   ```cmake
   if(CONFIG_APP_WAKEWORD_EDGE_IMPULSE)
       add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/edge-impulse edge-impulse)
       target_link_libraries(app PRIVATE edge_impulse_sdk)
   endif()
   ```

4. **Configure in prj.conf**
   ```
   CONFIG_APP_WAKEWORD_EDGE_IMPULSE=y
   CONFIG_APP_WAKEWORD_MODEL_PATH="ei_model"
   CONFIG_APP_WAKEWORD_FEATURE_SIZE=512
   ```

5. **Implement Model Loading**
   
   Update `loadEdgeImpulseModel()` in `wakeword_module.cpp`:
   ```cpp
   #include "edge_impulse.h"
   
   int WakeWordModule::loadEdgeImpulseModel() {
       // Initialize Edge Impulse impulse
       if (ei_impulse_init() != EI_IMPULSE_OK) {
           LOG_ERR("Failed to initialize Edge Impulse model");
           return -EIO;
       }
       
       LOG_INF("Edge Impulse model loaded successfully");
       LOG_INF("Input size: %d", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
       
       model_loaded_ = true;
       return 0;
   }
   ```

6. **Implement Inference**
   
   Update `runEdgeImpulseInference()`:
   ```cpp
   float WakeWordModule::runEdgeImpulseInference(const int16_t* samples, size_t count) {
       signal_t signal;
       signal.total_length = count;
       signal.get_data = [](size_t offset, size_t length, float *out_ptr) {
           // Copy audio data to output buffer
           return 0;
       };
       
       ei_impulse_result_t result = { 0 };
       EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
       
       if (res != EI_IMPULSE_OK) {
           LOG_ERR("Classification failed: %d", res);
           return 0.0f;
       }
       
       // Return confidence for wake word class (usually index 0 or 1)
       return result.classification[0].value;
   }
   ```

### 2. TensorFlow Lite Micro

TensorFlow Lite Micro is a lightweight ML framework for microcontrollers.

#### Prerequisites
- TensorFlow Lite Micro library
- Trained .tflite model file

#### Integration Steps

1. **Add TFLite Micro to west.yml**
   ```yaml
   - name: tensorflow
     url: https://github.com/tensorflow/tflite-micro
     revision: main
     path: modules/lib/tflite-micro
   ```

2. **Convert Your Model to TFLite**
   ```python
   import tensorflow as tf
   
   # Load your trained model
   model = tf.keras.models.load_model('wakeword_model.h5')
   
   # Convert to TFLite
   converter = tf.lite.TFLiteConverter.from_keras_model(model)
   converter.optimizations = [tf.lite.Optimize.DEFAULT]
   tflite_model = converter.convert()
   
   # Save
   with open('wakeword_model.tflite', 'wb') as f:
       f.write(tflite_model)
   ```

3. **Embed Model in Firmware**
   
   Convert model to C array:
   ```bash
   xxd -i wakeword_model.tflite > wakeword_model.h
   ```
   
   Add header to your project.

4. **Configure in prj.conf**
   ```
   CONFIG_APP_WAKEWORD_TFLITE=y
   CONFIG_APP_WAKEWORD_MODEL_PATH="g_wakeword_model"
   CONFIG_APP_WAKEWORD_ARENA_SIZE=8192
   CONFIG_APP_WAKEWORD_FEATURE_SIZE=512
   ```

5. **Implement Model Loading and Inference**
   
   See the stubs in `wakeword_module.cpp` for the integration pattern.

### 3. Placeholder (Energy-Based Detection)

The placeholder backend uses simple energy-based detection for testing.

#### Configuration
```
CONFIG_APP_WAKEWORD_PLACEHOLDER=y
```

This mode is useful for:
- Testing audio pipeline without a trained model
- Development and debugging
- Quick prototyping

**Note:** This is NOT real wake-word detection and will trigger on any loud sound.

## Configuration Options

All configuration is done via Kconfig in `software/app/Kconfig`.

| Option | Description | Default |
|--------|-------------|---------|
| `CONFIG_APP_WAKEWORD_EDGE_IMPULSE` | Use Edge Impulse SDK | No |
| `CONFIG_APP_WAKEWORD_TFLITE` | Use TensorFlow Lite Micro | No |
| `CONFIG_APP_WAKEWORD_PLACEHOLDER` | Use energy-based placeholder | Yes |
| `CONFIG_APP_WAKEWORD_MODEL_PATH` | Model path or symbol name | "ei_model" |
| `CONFIG_APP_WAKEWORD_FEATURE_SIZE` | Feature vector size | 512 |
| `CONFIG_APP_WAKEWORD_ARENA_SIZE` | TFLite arena size (bytes) | 8192 |

## Usage Example

```cpp
#include "modules/wakeword/wakeword_module.hpp"

void wake_word_detected(const WakeWordModule::DetectionInfo& info) {
    LOG_INF("Wake word '%s' detected with confidence %.2f", 
            info.keyword, info.confidence);
}

void setup_wakeword() {
    WakeWordModule& ww = WakeWordModule::getInstance();
    
    // Initialize (loads model)
    int ret = ww.init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize wake word module: %d", ret);
        return;
    }
    
    // Set detection callback
    ww.setDetectionCallback(wake_word_detected);
    
    // Set confidence threshold (0.0 - 1.0)
    ww.setThreshold(0.75f);
    
    // Start detection
    ww.start();
    
    // Process audio samples
    int16_t audio_buffer[512];
    // ... fill buffer from I2S microphone ...
    
    auto result = ww.process(audio_buffer, 512);
    if (result == WakeWordModule::DetectionResult::WAKE_WORD_DETECTED) {
        LOG_INF("Wake word detected!");
    }
}
```

## Performance Considerations

### Memory Usage
- **Edge Impulse**: Typically 20-100 KB flash, 10-50 KB RAM
- **TFLite Micro**: Depends on model size and arena configuration
- **Placeholder**: Negligible (<1 KB)

### Processing Time
- Aim for <50ms inference time to avoid audio buffer overflow
- Use quantized models (int8) for better performance
- Consider model complexity vs. accuracy trade-offs

### Power Consumption
- Wake-word detection runs continuously
- Optimize model size and complexity for battery-powered devices
- Consider using lower sample rates (8 kHz or 16 kHz)

## Troubleshooting

### Model Loading Fails
- Check that the model file or symbol exists
- Verify model format matches backend (Edge Impulse vs. TFLite)
- Check memory constraints (flash and RAM)

### Low Detection Accuracy
- Adjust confidence threshold with `setThreshold()`
- Verify audio preprocessing matches training conditions
- Check sample rate and bit depth settings
- Retrain model with more diverse samples

### Performance Issues
- Reduce model complexity
- Use quantized models
- Optimize audio preprocessing
- Check for memory leaks or buffer overflows

## Resources

- [Edge Impulse Documentation](https://docs.edgeimpulse.com/)
- [TensorFlow Lite Micro Guide](https://www.tensorflow.org/lite/microcontrollers)
- [Zephyr RTOS Documentation](https://docs.zephyrproject.org/)
- [Audio Classification Tutorial](https://docs.edgeimpulse.com/docs/tutorials/audio-classification)

## License

Copyright (c) 2025 Sprchuoi  
SPDX-License-Identifier: Apache-2.0
