# Model Management Guide

This guide explains how to add, update, and manage wake-word detection models in your repository.

## Quick Answer

**Option 1: Add Model to This Repository (Recommended for small models)**
- Convert model to C header file
- Add `model_data.h` to the wakeword directory
- The `.gitignore` is already configured to exclude it
- If you want to commit it: Remove from `.gitignore` first

**Option 2: Separate Model Repository (Recommended for large/proprietary models)**
- Keep models in a separate private repository
- Use git submodules or manual download
- Keep this codebase clean and model-agnostic

**Option 3: Model Registry/Storage (Production approach)**
- Store models in cloud storage (S3, Azure Blob, etc.)
- Download during build or at runtime
- Version control via registry

## Detailed Workflows

### Workflow 1: Embed Model in This Repository

For small models (< 100KB) that you want versioned with your code:

#### Step 1: Prepare Your Model
```bash
# Export from Edge Impulse and convert to C header
xxd -i your_wakeword_model.tflite > model_data.h
```

#### Step 2: Edit model_data.h
The `xxd` command creates variables like `your_wakeword_model_tflite[]`. Rename them:
```cpp
// Change this:
unsigned char your_wakeword_model_tflite[] = { ... };
unsigned int your_wakeword_model_tflite_len = 12345;

// To this:
unsigned char g_model_data[] = { ... };
unsigned int g_model_data_len = 12345;
```

#### Step 3: Add to Repository (Choose One)

**Option A: Keep Model Private (Default)**
```bash
# .gitignore already excludes model_data.h, so just add the file:
cp model_data.h software/app/src/modules/wakeword/

# It will NOT be committed by default
# Each developer adds their own model_data.h locally
```

**Option B: Commit Model to Repository**
```bash
# Remove model_data.h from .gitignore first
sed -i '/model_data.h/d' .gitignore

# Add and commit the model
git add software/app/src/modules/wakeword/model_data.h
git commit -m "Add wake-word detection model"
git push
```

#### Step 4: Update Code to Use Model
Uncomment the model loading code in `model_loader.cpp` (lines ~97-101):
```cpp
#include "model_data.h"

// In EdgeImpulseModelLoader::load()
model_data_ = g_model_data;
model_size_ = g_model_data_len;
```

#### Step 5: Build and Test
```bash
cd software
./make.sh build -- -DCONF_FILE=voice.conf -DCONFIG_APP_WAKEWORD_MODEL_EDGE_IMPULSE=y
./make.sh flash
```

### Workflow 2: Separate Model Repository

For large models, proprietary models, or when multiple projects share models:

#### Step 1: Create Model Repository
```bash
# Create a new repository for models only
mkdir wakeword-models
cd wakeword-models
git init

# Add your model
cp /path/to/model_data.h .
git add model_data.h
git commit -m "Add initial wake-word model v1.0"
git remote add origin git@github.com:yourorg/wakeword-models.git
git push -u origin main
```

#### Step 2: Add as Git Submodule
```bash
# In your main project
cd /path/to/smart_home_zephyr
git submodule add git@github.com:yourorg/wakeword-models.git software/app/src/modules/wakeword/models

# Update .gitignore to track submodule but not individual model files
echo "!software/app/src/modules/wakeword/models/" >> .gitignore
```

#### Step 3: Update Build System
Create a symlink or modify CMakeLists.txt:
```bash
# Create symlink to current model
cd software/app/src/modules/wakeword
ln -s models/model_data.h model_data.h
```

Or modify `CMakeLists.txt`:
```cmake
# Add include path for models submodule
zephyr_library_include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../..
  ${CMAKE_CURRENT_SOURCE_DIR}/models  # Add this line
)
```

#### Step 4: Clone Project with Models
```bash
# New developers clone with submodules
git clone --recursive git@github.com:yourorg/smart_home_zephyr.git

# Or initialize submodules after clone
git clone git@github.com:yourorg/smart_home_zephyr.git
cd smart_home_zephyr
git submodule update --init --recursive
```

### Workflow 3: Runtime Model Loading (Advanced)

For production systems that download models at runtime:

#### Step 1: Store Models Externally
- Upload to cloud storage (S3, Azure, Google Cloud)
- Set up HTTP server with model files
- Use versioned URLs (e.g., `https://models.example.com/wakeword/v1.2.0/model.tflite`)

#### Step 2: Modify Code for External Loading
Update `model_loader.cpp` to support HTTP download:
```cpp
#ifdef CONFIG_APP_WAKEWORD_MODEL_EXTERNAL_URL

int EdgeImpulseModelLoader::load() {
    // Download model from CONFIG_APP_WAKEWORD_MODEL_URL
    // Cache to flash storage
    // Load from flash
}

#endif
```

Add Kconfig option:
```kconfig
config APP_WAKEWORD_MODEL_URL
    string "Model download URL"
    default "https://your-server.com/models/wakeword.tflite"
```

## Retraining and Model Updates

### When to Retrain
- Poor accuracy with real-world data
- New wake-word requirements
- Environmental noise changes
- Hardware changes (different microphone)

### Update Workflow

#### For Embedded Models (Workflow 1)
```bash
# 1. Retrain in Edge Impulse Studio
# 2. Export new model
# 3. Convert to C header
xxd -i new_model.tflite > model_data.h

# 4. Replace old model
cp model_data.h software/app/src/modules/wakeword/

# 5. Test locally
cd software
./make.sh clean
./make.sh build -- -DCONF_FILE=voice.conf
./make.sh flash
./make.sh monitor

# 6. If working, commit (if model is tracked in git)
git add software/app/src/modules/wakeword/model_data.h
git commit -m "Update wake-word model to v1.1 - improved accuracy"
git push
```

#### For Separate Repository (Workflow 2)
```bash
# 1. Update model in models repo
cd wakeword-models
cp /path/to/new_model_data.h model_data.h
git add model_data.h
git commit -m "Model v1.1 - improved noise rejection"
git tag v1.1.0
git push origin main --tags

# 2. Update submodule reference in main repo
cd /path/to/smart_home_zephyr/software/app/src/modules/wakeword/models
git pull origin main
cd ../../../../..
git add software/app/src/modules/wakeword/models
git commit -m "Update wake-word model to v1.1.0"
git push
```

## Best Practices

### Model Versioning
```cpp
// Add version info to model_data.h
#define MODEL_VERSION "1.2.0"
#define MODEL_TRAIN_DATE "2025-12-11"
#define MODEL_ACCURACY 0.94f

// Log on load
LOG_INF("Model version: %s, trained: %s, accuracy: %.2f", 
        MODEL_VERSION, MODEL_TRAIN_DATE, MODEL_ACCURACY);
```

### Model Size Management
- Keep models < 50KB for embedded devices
- Use int8 quantization (4x smaller than float32)
- Test on target hardware, not just simulation

### Testing After Updates
```bash
# Always test after model updates
./make.sh build && ./make.sh flash

# Monitor and verify:
# 1. Model loads successfully
# 2. Inference latency acceptable (< 100ms)
# 3. Wake-word detection accuracy
# 4. False positive rate acceptable
```

### Git LFS for Large Models
If models are > 1MB, use Git Large File Storage:
```bash
# Install git-lfs
git lfs install

# Track model files
git lfs track "*.tflite"
git lfs track "model_data.h"

# Add .gitattributes
git add .gitattributes
git commit -m "Track models with Git LFS"

# Add and commit model
git add model_data.h
git commit -m "Add model v1.0"
git push
```

## Decision Matrix

| Criterion | Workflow 1 (Embedded) | Workflow 2 (Submodule) | Workflow 3 (Runtime) |
|-----------|----------------------|------------------------|----------------------|
| **Model Size** | < 100KB | Any size | Any size |
| **Update Frequency** | Low | Medium-High | High |
| **Privacy** | Public/Private | Private | Private |
| **Build Complexity** | Low | Medium | High |
| **Runtime Flexibility** | No | No | Yes |
| **Internet Required** | No | No | Yes (first run) |
| **Recommended For** | Open source, small | Teams, versioned | Production, A/B testing |

## Example: Complete Setup

### Scenario: Small team with proprietary model

```bash
# 1. Create private model repo
mkdir ../wakeword-models
cd ../wakeword-models
git init
echo "# Wake-Word Models (Private)" > README.md
git add README.md
git commit -m "Initial commit"
gh repo create yourorg/wakeword-models --private --source=. --push

# 2. Add your model
xxd -i wakeword_v1.tflite > model_data.h
# Edit to rename variables to g_model_data
git add model_data.h
git commit -m "Add wake-word model v1.0.0"
git tag v1.0.0
git push origin main --tags

# 3. Add to main project as submodule
cd /path/to/smart_home_zephyr
git submodule add git@github.com:yourorg/wakeword-models.git \
    software/app/src/modules/wakeword/models

# 4. Create symlink
cd software/app/src/modules/wakeword
ln -s models/model_data.h model_data.h

# 5. Commit submodule
cd ../../../../..
git add .gitmodules software/app/src/modules/wakeword/models
git commit -m "Add model repository as submodule"
git push

# 6. Team members clone
git clone --recursive git@github.com:yourorg/smart_home_zephyr.git
```

## Troubleshooting

### "model_data.h not found"
- Check file exists: `ls software/app/src/modules/wakeword/model_data.h`
- Check include path in CMakeLists.txt
- Try absolute path: `#include "software/app/src/modules/wakeword/model_data.h"`

### "Model too large for flash"
- Use int8 quantization in Edge Impulse
- Reduce model complexity (fewer layers/neurons)
- Check flash partition sizes in Kconfig

### "Submodule not cloning"
```bash
# Initialize and update submodules
git submodule update --init --recursive

# Or clone with submodules
git clone --recursive <repo-url>
```

## Summary

**Quick recommendation:**
- **Start with Workflow 1** (embed in repo): Fast, simple, works immediately
- **Graduate to Workflow 2** (submodule): When model grows or team expands
- **Move to Workflow 3** (runtime): For production with OTA model updates

The current implementation supports all three approaches - choose based on your needs!
