# Pulse Radar System Build
PROJECT_NAME = pulse_radar_system
BOARD = pynq-z1
VIVADO_VERSION = 2023.1

# Directories
BUILD_DIR = build
VIVADO_DIR = vivado_project
PETALINUX_DIR = petalinux_project
HDL_DIR = radar_ip/hdl
CONSTRAINTS_DIR = constraints
DRIVER_DIR = project-spec/meta-user/recipes-modules/radar_driver
APP_DIR = project-spec/meta-user/recipes-apps/radar_app

# Vivado settings
VIVADO_SCRIPT = scripts/build_vivado.tcl
VIVADO_BATCH = vivado -mode batch -source

# PetaLinux settings
PETALINUX_CONFIG = project-spec/configs/config
PETALINUX_ROOTFS = project-spec/configs/rootfs_config

.PHONY: all clean vivado petalinux driver app bitstream boot

all: bitstream boot

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Vivado project creation and build
vivado: $(BUILD_DIR)
	@echo "Building Vivado project..."
	$(VIVADO_BATCH) $(VIVADO_SCRIPT) -tclargs $(PROJECT_NAME) $(BOARD)
	@echo "Vivado project built successfully"

# Generate bitstream
bitstream: vivado
	@echo "Generating bitstream..."
	cd $(VIVADO_DIR) && vivado -mode batch -source ../scripts/generate_bitstream.tcl
	cp $(VIVADO_DIR)/$(PROJECT_NAME).runs/impl_1/design_1_wrapper.bit $(BUILD_DIR)/system.bit
	@echo "Bitstream generated successfully"

# PetaLinux project setup
petalinux-setup:
	@echo "Setting up PetaLinux project..."
	petalinux-create -t project --template zynq --name $(PROJECT_NAME)
	cd $(PROJECT_NAME) && petalinux-config --get-hw-description=../$(VIVADO_DIR)/$(PROJECT_NAME).sdk/
	@echo "PetaLinux project setup complete"

# Build PetaLinux
petalinux: petalinux-setup driver app
	@echo "Building PetaLinux..."
	cd $(PROJECT_NAME) && petalinux-build
	@echo "PetaLinux built successfully"

# Build kernel driver
driver:
	@echo "Building radar driver..."
	cd $(PROJECT_NAME) && petalinux-create -t modules --name radar_driver
	cp $(DRIVER_DIR)/files/* $(PROJECT_NAME)/project-spec/meta-user/recipes-modules/radar_driver/files/
	cd $(PROJECT_NAME) && petalinux-build -c radar_driver
	@echo "Radar driver built successfully"

# Build user application
app:
	@echo "Building radar application..."
	cd $(PROJECT_NAME) && petalinux-create -t apps --name radar_app
	cp $(APP_DIR)/files/* $(PROJECT_NAME)/project-spec/meta-user/recipes-apps/radar_app/files/
	cd $(PROJECT_NAME) && petalinux-build -c radar_app
	@echo "Radar application built successfully"

# Generate boot files
boot: petalinux bitstream
	@echo "Generating boot files..."
	cd $(PROJECT_NAME) && petalinux-package --boot --fsbl images/linux/zynq_fsbl.elf --fpga ../$(BUILD_DIR)/system.bit --u-boot
	cp $(PROJECT_NAME)/images/linux/BOOT.BIN $(BUILD_DIR)/
	cp $(PROJECT_NAME)/images/linux/image.ub $(BUILD_DIR)/
	cp $(PROJECT_NAME)/images/linux/rootfs.tar.gz $(BUILD_DIR)/
	@echo "Boot files generated successfully"

# Create SD card image
sdcard: boot
	@echo "Creating SD card image..."
	scripts/create_sdcard.sh $(BUILD_DIR)
	@echo "SD card image created successfully"

# Clean build
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(VIVADO_DIR)
	rm -rf $(PROJECT_NAME)
	@echo "Clean completed"

# Help
help:
	@echo "Pulse Radar System Build Targets:"
	@echo "  all        - Build complete system"
	@echo "  vivado     - Create Vivado project"
	@echo "  bitstream  - Generate FPGA bitstream"
	@echo "  petalinux  - Build PetaLinux project"
	@echo "  driver     - Build kernel driver"
	@echo "  app        - Build user application"
	@echo "  boot       - Generate boot files"
	@echo "  sdcard     - Create SD card image"
	@echo "  clean      - Clean all build files"
