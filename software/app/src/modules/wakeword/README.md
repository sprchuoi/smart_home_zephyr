# Wake-Word Detection Model Integration

This directory contains the wake-word detection module with support for multiple model types.

## Supported Model Types

### 1. Placeholder (Default)
Simple energy-based detection for testing without a real ML model.

**Configuration:**
```kconfig
CONFIG_APP_WAKEWORD_MODEL_PLACEHOLDER=y
```

**Use case:** Testing the audio pipeline and integration without ML dependencies.

### 2. Edge Impulse
TensorFlow Lite for Microcontrollers models trained with Edge Impulse Studio.

**Configuration:**
```kconfig
CONFIG_APP_WAKEWORD_MODEL_EDGE_IMPULSE=y
CONFIG_APP_WAKEWORD_MODEL_EMBEDDED=y
CONFIG_APP_WAKEWORD_ARENA_SIZE=8192
```

**Requirements:**
- Edge Impulse account and trained model
- TensorFlow Lite for Microcontrollers library
- Exported model as C array

**Integration steps:**

1. **Train your model in Edge Impulse Studio:**
   - Create a project at https://studio.edgeimpulse.com/
   - Upload audio samples with wake-word and background noise
   - Create impulse with audio preprocessing (MFCC recommended)
   - Train your model
   - Test and validate accuracy

2. **Export the model:**
   - Go to "Deployment" in Edge Impulse Studio
   - Select "TensorFlow Lite (int8)" or "TensorFlow Lite (float32)"
   - Download the model

3. **Convert model to C array:**
   ```bash
   xxd -i model.tflite > model_data.h
   ```

4. **Integrate into project:**
   - Create file: `software/app/src/modules/wakeword/model_data.h`
   - Include the model data array
   - Uncomment model loading code in `model_loader.cpp`:

   ```cpp
   #include "model_data.h"
   
   // In EdgeImpulseModelLoader::load()
   model_data_ = g_model_data;
   model_size_ = g_model_data_len;
   ```

5. **Enable TensorFlow Lite Micro:**
   Add to `prj.conf` or `voice.conf`:
   ```kconfig
   CONFIG_TENSORFLOW_LITE_MICRO=y
   ```
   
   Note: You may need to add TensorFlow Lite Micro as a Zephyr module in your `west.yml`.

6. **Implement TFLite inference:**
   The model_loader.cpp contains TODO comments for TFLite integration:
   - Include TFLite headers
   - Initialize model with `tflite::GetModel()`
   - Create operation resolver
   - Build interpreter
   - Allocate tensors
   - Implement inference in `infer()` method

### 3. Custom Model
For user-defined model formats (e.g., proprietary formats, other frameworks).

**Configuration:**
```kconfig
CONFIG_APP_WAKEWORD_MODEL_CUSTOM=y
CONFIG_APP_WAKEWORD_MODEL_FILE="path/to/model.bin"
```

**Implementation:**
Modify `CustomModelLoader` class in `model_loader.cpp` to support your format:
- Implement `load()` to read and parse model
- Implement `infer()` to run inference
- Handle any custom preprocessing/postprocessing

## Model Performance Considerations

### Memory Requirements
- **Placeholder:** ~0 bytes (no model)
- **Edge Impulse (typical):** 10-50 KB model + 8-32 KB arena
- **Custom:** Depends on implementation

### Latency
- Target: < 100ms per inference
- Window size: 512 samples (32ms at 16kHz)
- Adjust `CONFIG_APP_WAKEWORD_ARENA_SIZE` based on model complexity

### Accuracy vs Resource Trade-off
- Smaller models: Faster, less accurate
- Larger models: More accurate, higher latency and memory
- Quantized models (int8): 4x smaller than float32 with minimal accuracy loss

## Testing Your Model

1. **Enable logging:**
   ```kconfig
   CONFIG_APP_LOG_LEVEL=4  # Debug level
   ```

2. **Monitor detection:**
   ```bash
   ./make.sh monitor
   ```

3. **Test with audio:**
   - Speak your wake-word near the microphone
   - Observe confidence scores in logs
   - Adjust threshold with `setThreshold()`

4. **Tune threshold:**
   - Start with 0.7 (default)
   - Lower for better sensitivity (more false positives)
   - Higher for better specificity (fewer false positives)

## Feature Extraction

Current implementation uses simple normalization. For better accuracy:

1. **Implement MFCC (Mel-Frequency Cepstral Coefficients):**
   - More robust to variations in speech
   - Standard feature for speech recognition
   - Edge Impulse models typically expect MFCC features

2. **Update `preprocessAudio()` in `wakeword_module.cpp`:**
   ```cpp
   void preprocessAudio(const int16_t* samples, size_t count, float* features) {
       // TODO: Apply FFT, MFCC, or other feature extraction
       // Example: Use CMSIS-DSP for FFT
       // Then compute Mel filterbank energies
       // Apply DCT for cepstral coefficients
   }
   ```

## Debugging

### Model won't load
- Check `CONFIG_APP_WAKEWORD_MODEL_*` settings
- Verify model data is included in build
- Check arena size is sufficient
- Enable debug logs

### Low accuracy
- Verify feature extraction matches training
- Check input tensor dimensions
- Test with known good samples
- Retrain model with more diverse data

### High memory usage
- Reduce `CONFIG_APP_WAKEWORD_ARENA_SIZE`
- Use int8 quantized model
- Optimize model architecture (fewer layers/neurons)

## Example Configuration Files

### Edge Impulse with embedded model
Create `voice_ei.conf`:
```kconfig
CONFIG_APP_VOICE_CONTROL=y
CONFIG_APP_WAKEWORD=y
CONFIG_APP_WAKEWORD_MODEL_EDGE_IMPULSE=y
CONFIG_APP_WAKEWORD_MODEL_EMBEDDED=y
CONFIG_APP_WAKEWORD_ARENA_SIZE=16384
CONFIG_TENSORFLOW_LITE_MICRO=y
CONFIG_FPU=y
```

Build with:
```bash
./make.sh build -- -DCONF_FILE=voice_ei.conf
```

## Resources

- [Edge Impulse Documentation](https://docs.edgeimpulse.com/)
- [TensorFlow Lite Micro](https://www.tensorflow.org/lite/microcontrollers)
- [Zephyr TFLite Integration](https://docs.zephyrproject.org/latest/samples/modules/tflite-micro/hello_world/README.html)
- [Audio Feature Extraction](https://en.wikipedia.org/wiki/Mel-frequency_cepstrum)

## Contributing

To add support for additional model frameworks:
1. Create new model loader class in `model_loader.cpp`
2. Add Kconfig option in `Kconfig`
3. Update factory function `createModelLoader()`
4. Document usage in this README
