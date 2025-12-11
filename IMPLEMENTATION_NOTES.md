# Wake Word Module - Quick Start Guide

## Overview

The wake word detection module has been updated to support multiple ML backends:
- **Edge Impulse SDK** (recommended for ease of use)
- **TensorFlow Lite Micro** (for custom models)
- **Placeholder** (energy-based detection for testing)

## What Was Changed

### 1. Configuration System (`software/app/Kconfig`)
Added Kconfig options to select ML backend:
- `CONFIG_APP_WAKEWORD_EDGE_IMPULSE`: Enable Edge Impulse
- `CONFIG_APP_WAKEWORD_TFLITE`: Enable TensorFlow Lite
- `CONFIG_APP_WAKEWORD_PLACEHOLDER`: Enable placeholder (default)
- `CONFIG_APP_WAKEWORD_MODEL_PATH`: Model path or symbol
- `CONFIG_APP_WAKEWORD_FEATURE_SIZE`: Feature vector size
- `CONFIG_APP_WAKEWORD_ARENA_SIZE`: TFLite arena size

### 2. Module Header (`wakeword_module.hpp`)
Added methods:
- `loadEdgeImpulseModel()`: Load Edge Impulse model
- `loadTFLiteModel()`: Load TFLite model
- `loadPlaceholderModel()`: Load placeholder
- `runEdgeImpulseInference()`: Run EI inference
- `runTFLiteInference()`: Run TFLite inference
- `runPlaceholderInference()`: Run placeholder inference
- `getBackendType()`: Get current backend name
- `getDetectionCount()`: Get detection statistics

Added member variables for ML contexts:
- `tflite_interpreter_`: TFLite interpreter context
- `tflite_tensor_arena_`: TFLite memory arena
- `ei_impulse_`: Edge Impulse context

### 3. Module Implementation (`wakeword_module.cpp`)
- Implemented backend selection with conditional compilation
- Added detailed logging for model loading
- Implemented stub functions with integration patterns
- Added error handling for unsupported backends
- Updated inference routing based on selected backend

### 4. Documentation
- Created `README.md`: Comprehensive integration guide
- Created `model_data_template.h`: Template for embedded models
- Updated `voice_system.rst`: Documentation for backend selection

## Current Status

✅ **Completed:**
- Configuration system with Kconfig
- Backend selection and routing
- Stub implementations with integration patterns
- Comprehensive documentation
- Error handling and logging
- Backward compatibility (defaults to placeholder)

⚠️ **Requires User Action:**
To actually load and run a trained model, users must:
1. Choose a backend (Edge Impulse or TFLite)
2. Train a model or obtain a pre-trained one
3. Link the ML library (Edge Impulse SDK or TFLite Micro)
4. Uncomment the header includes in `wakeword_module.cpp`
5. Implement the model loading in the appropriate function
6. Implement the inference in the appropriate function

## Testing Without ML Libraries

The module works out-of-the-box with the placeholder backend:

```bash
# Default configuration uses placeholder
cd software
./make.sh build
./make.sh flash
./make.sh monitor
```

Expected log output:
```
[INF] wakeword: Initializing wake word detection module
[INF] wakeword: Backend: Placeholder (Energy-Based)
[WRN] wakeword: Using placeholder model (energy-based detection)
[INF] wakeword: This is a simple energy detector, not a trained wake-word model
[INF] wakeword: To use real wake-word detection:
[INF] wakeword:   1. Configure CONFIG_APP_WAKEWORD_EDGE_IMPULSE or CONFIG_APP_WAKEWORD_TFLITE
[INF] wakeword:   2. Link the appropriate ML library
[INF] wakeword:   3. Provide a trained model
[INF] wakeword: Wake word module initialized, threshold: 0.70
```

## Integration Path

### For Edge Impulse:

1. **Configure:**
   ```
   CONFIG_APP_WAKEWORD_EDGE_IMPULSE=y
   CONFIG_APP_WAKEWORD_MODEL_PATH="ei_model"
   ```

2. **Link SDK:**
   Add Edge Impulse SDK to `lib/edge-impulse/`

3. **Implement:**
   Uncomment and complete in `wakeword_module.cpp`:
   ```cpp
   #include "edge_impulse.h"
   
   int WakeWordModule::loadEdgeImpulseModel() {
       // Initialize impulse
       if (ei_impulse_init() != EI_IMPULSE_OK) {
           LOG_ERR("Failed to initialize Edge Impulse model");
           return -EIO;
       }
       model_loaded_ = true;
       return 0;
   }
   
   float WakeWordModule::runEdgeImpulseInference(...) {
       // Run classifier
       ei_impulse_result_t result = { 0 };
       run_classifier(&signal, &result, false);
       return result.classification[0].value;
   }
   ```

### For TensorFlow Lite:

1. **Configure:**
   ```
   CONFIG_APP_WAKEWORD_TFLITE=y
   CONFIG_APP_WAKEWORD_MODEL_PATH="g_wakeword_model"
   CONFIG_APP_WAKEWORD_ARENA_SIZE=8192
   ```

2. **Embed Model:**
   ```bash
   xxd -i model.tflite > model_data.h
   ```

3. **Implement:**
   Uncomment and complete TFLite code in `wakeword_module.cpp`

## Verification

To verify the implementation works correctly:

1. **Check Configuration:**
   ```bash
   west build -t menuconfig
   # Navigate to: Voice Control -> Wake-word detection
   ```

2. **Check Compilation:**
   ```bash
   cd software
   ./make.sh build
   # Should compile without errors
   ```

3. **Check Runtime:**
   ```bash
   ./make.sh flash
   ./make.sh monitor
   # Should see backend selection in logs
   ```

4. **Check Backend Selection:**
   - Default: "Backend: Placeholder (Energy-Based)"
   - Edge Impulse: "Backend: Edge Impulse"
   - TFLite: "Backend: TensorFlow Lite Micro"

## Benefits

✅ **Flexibility**: Support for multiple ML frameworks
✅ **Modularity**: Clean separation between backends
✅ **Testability**: Works without ML libraries (placeholder)
✅ **Documentation**: Comprehensive guides for integration
✅ **Future-proof**: Easy to add new backends
✅ **Error Handling**: Clear error messages and logging

## Files Changed

- `software/app/Kconfig` - Added ML backend configuration
- `software/app/src/modules/wakeword/wakeword_module.hpp` - Added backend methods
- `software/app/src/modules/wakeword/wakeword_module.cpp` - Implemented backend logic
- `software/doc/voice_system.rst` - Updated documentation
- `software/app/src/modules/wakeword/README.md` - New integration guide
- `software/app/src/modules/wakeword/model_data_template.h` - New template file

## Next Steps for Users

1. Review the integration guide in `README.md`
2. Choose a backend (Edge Impulse recommended)
3. Train a wake-word model
4. Follow integration steps in the guide
5. Test and tune detection threshold

For questions or issues, see the documentation or file an issue.
