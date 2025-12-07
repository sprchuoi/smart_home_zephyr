#!/bin/bash

# ESP32 Smart Home - Setup Zephyr Workspace
# This script sets up the Zephyr environment for building

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}ESP32 Smart Home - Zephyr Setup${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

# Check if running in a Zephyr workspace
if [ -n "$ZEPHYR_BASE" ]; then
    echo -e "${GREEN}✓ ZEPHYR_BASE is set: $ZEPHYR_BASE${NC}"
else
    echo -e "${YELLOW}⚠ ZEPHYR_BASE not set${NC}"
    echo ""
    echo "You need to setup Zephyr first. Choose an option:"
    echo ""
    echo "1. Create new Zephyr workspace (recommended)"
    echo "2. Use existing Zephyr installation"
    echo "3. Exit and setup manually"
    echo ""
    read -p "Enter choice [1-3]: " choice
    
    case $choice in
        1)
            echo ""
            echo -e "${BLUE}Creating Zephyr workspace...${NC}"
            
            # Create workspace
            WORKSPACE_DIR="$HOME/zephyrproject"
            mkdir -p $WORKSPACE_DIR
            cd $WORKSPACE_DIR
            
            # Check if west is installed
            if ! command -v west &> /dev/null; then
                echo -e "${YELLOW}Installing west...${NC}"
                
                # Try --user first
                if pip3 install --user west 2>/dev/null; then
                    echo -e "${GREEN}✓ West installed${NC}"
                elif pip3 install --user --break-system-packages west 2>/dev/null; then
                    echo -e "${GREEN}✓ West installed${NC}"
                else
                    echo -e "${RED}✗ Failed to install west${NC}"
                    echo "Try manually: pip3 install --user --break-system-packages west"
                    exit 1
                fi
                
                export PATH=$PATH:~/.local/bin
            fi
            
            # Initialize workspace
            if [ ! -f ".west/config" ]; then
                echo -e "${YELLOW}Initializing west workspace...${NC}"
                west init
            fi
            
            # Update modules
            echo -e "${YELLOW}Updating Zephyr modules (this may take a while)...${NC}"
            west update
            
            # Export Zephyr environment
            echo -e "${YELLOW}Exporting Zephyr environment...${NC}"
            west zephyr-export
            
            # Install Python dependencies
            echo -e "${YELLOW}Installing Python dependencies...${NC}"
            
            # Try with --user flag first (works on most systems)
            if pip3 install --user -r $WORKSPACE_DIR/zephyr/scripts/requirements.txt 2>/dev/null; then
                echo -e "${GREEN}✓ Python dependencies installed${NC}"
            else
                # If that fails, suggest alternatives
                echo -e "${YELLOW}Note: System Python is externally managed${NC}"
                echo -e "${YELLOW}Using --break-system-packages flag for dependencies...${NC}"
                
                if pip3 install --user --break-system-packages -r $WORKSPACE_DIR/zephyr/scripts/requirements.txt; then
                    echo -e "${GREEN}✓ Python dependencies installed${NC}"
                else
                    echo -e "${YELLOW}⚠ Could not install some Python dependencies${NC}"
                    echo -e "${YELLOW}You may need to install them manually or use a venv${NC}"
                    echo ""
                    echo "Alternative: Create a virtual environment:"
                    echo "  python3 -m venv ~/.zephyr-venv"
                    echo "  source ~/.zephyr-venv/bin/activate"
                    echo "  pip install -r $WORKSPACE_DIR/zephyr/scripts/requirements.txt"
                    echo ""
                    echo "Press Enter to continue anyway..."
                    read
                fi
            fi
            
            # Set environment variables
            export ZEPHYR_BASE=$WORKSPACE_DIR/zephyr
            
            echo ""
            echo -e "${GREEN}✓ Zephyr workspace created at: $WORKSPACE_DIR${NC}"
            echo ""
            echo -e "${YELLOW}Add these to your ~/.zshrc:${NC}"
            echo "export ZEPHYR_BASE=$WORKSPACE_DIR/zephyr"
            echo 'export PATH=$PATH:~/.local/bin'
            echo ""
            ;;
        2)
            echo ""
            read -p "Enter path to Zephyr installation: " zephyr_path
            if [ -d "$zephyr_path" ]; then
                export ZEPHYR_BASE=$zephyr_path
                echo -e "${GREEN}✓ Using Zephyr from: $ZEPHYR_BASE${NC}"
            else
                echo -e "${RED}✗ Directory not found: $zephyr_path${NC}"
                exit 1
            fi
            ;;
        3)
            echo ""
            echo "Manual setup instructions:"
            echo "1. Install west: pip3 install --user west"
            echo "2. Create workspace: west init ~/zephyrproject && cd ~/zephyrproject"
            echo "3. Update: west update"
            echo "4. Export: west zephyr-export"
            echo "5. Set ZEPHYR_BASE: export ZEPHYR_BASE=~/zephyrproject/zephyr"
            exit 0
            ;;
        *)
            echo -e "${RED}Invalid choice${NC}"
            exit 1
            ;;
    esac
fi

# Check for ESP32 toolchain
echo ""
echo -e "${BLUE}Checking ESP32 toolchain...${NC}"
if command -v xtensa-esp32-elf-gcc &> /dev/null; then
    echo -e "${GREEN}✓ ESP32 toolchain found${NC}"
    xtensa-esp32-elf-gcc --version | head -1
else
    echo -e "${YELLOW}⚠ ESP32 toolchain not found${NC}"
    echo ""
    echo "Download and install:"
    echo "1. Download: wget https://github.com/espressif/crosstool-NG/releases/download/esp-2022r1/xtensa-esp32-elf-gcc11_2_0-esp-2022r1-linux-amd64.tar.xz"
    echo "2. Extract: tar -xf xtensa-esp32-elf-gcc11_2_0-esp-2022r1-linux-amd64.tar.xz -C ~"
    echo "3. Add to PATH: export PATH=\$PATH:~/xtensa-esp32-elf/bin"
    echo ""
fi

# Check west
echo ""
echo -e "${BLUE}Checking west...${NC}"
if command -v west &> /dev/null; then
    echo -e "${GREEN}✓ West found${NC}"
    west --version
else
    echo -e "${RED}✗ West not found${NC}"
    echo "Install: pip3 install --user west"
    exit 1
fi

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Setup complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo -e "${YELLOW}Important: Add these to your ~/.zshrc:${NC}"
echo ""
echo "export ZEPHYR_BASE=$ZEPHYR_BASE"
echo 'export PATH=$PATH:~/.local/bin'
if [ -d ~/xtensa-esp32-elf/bin ]; then
    echo 'export PATH=$PATH:~/xtensa-esp32-elf/bin'
fi
echo ""
echo -e "${YELLOW}Then run:${NC}"
echo "  source ~/.zshrc"
echo ""
echo -e "${YELLOW}Next steps:${NC}"
echo "1. Source your shell: source ~/.zshrc"
echo "2. Build project: ./make.sh build"
echo "3. Flash to ESP32: ./make.sh flash"
