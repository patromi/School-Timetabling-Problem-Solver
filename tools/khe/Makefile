# Makefile for compiling the KHE suite under Linux / WSL / Docker
# Jeffrey H. Kingston's KHE Timetabling Suite & HSEval

KHE_DIR ?= khe-2025_12_04
BIN_DIR ?= $(CURDIR)/bin

.PHONY: all build clean hseval khe help

all: build

help:
	@echo "Available targets:"
	@echo "  make build   - Compiles KHE platform, solvers, and produces bin/hseval and bin/khe"
	@echo "  make hseval  - Compiles only the HSEval evaluator (bin/hseval)"
	@echo "  make khe     - Compiles the full KHE solver (bin/khe)"
	@echo "  make clean   - Cleans all compiled objects and bin directory"

build: hseval khe
	@echo ""
	@echo "==> Build complete. Executables ready in $(BIN_DIR):"
	@ls -la $(BIN_DIR)

hseval:
	@mkdir -p $(BIN_DIR)
	$(MAKE) -C $(KHE_DIR)/src_platform
	$(MAKE) -C $(KHE_DIR)/src_hseval FINAL_DIR=$(BIN_DIR)
	@cp -f $(KHE_DIR)/src_hseval/hseval.cgi $(BIN_DIR)/hseval
	@chmod +x $(BIN_DIR)/hseval
	@echo "==> hseval compiled successfully."

khe:
	@mkdir -p $(BIN_DIR)
	$(MAKE) -C $(KHE_DIR)/src_platform
	$(MAKE) -C $(KHE_DIR)/src_solvers
	$(MAKE) -C $(KHE_DIR)/src_khe FINAL_DIR=$(BIN_DIR)
	@cp -f $(KHE_DIR)/src_khe/khe $(BIN_DIR)/khe
	@chmod +x $(BIN_DIR)/khe
	@echo "==> khe compiled successfully."

clean:
	-$(MAKE) -C $(KHE_DIR)/src_platform clean
	-$(MAKE) -C $(KHE_DIR)/src_solvers clean
	-$(MAKE) -C $(KHE_DIR)/src_hseval clean
	-$(MAKE) -C $(KHE_DIR)/src_khe clean
	-rm -rf $(BIN_DIR)
	@echo "==> Clean complete."
