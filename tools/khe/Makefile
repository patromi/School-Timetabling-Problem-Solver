# Makefile for compiling the KHE suite under Linux / WSL / Docker
# Jeffrey H. Kingston's KHE Timetabling Suite & HSEval

KHE_VERSION ?= khe-2025_12_04
KHE_TAR     ?= $(KHE_VERSION).tar.gz
KHE_URL     ?= http://jeffreykingston.id.au/khe/$(KHE_TAR)
KHE_DIR     ?= $(CURDIR)/$(KHE_VERSION)
BIN_DIR     ?= $(CURDIR)/bin

.PHONY: all build clean distclean fetch hseval khe help

all: build

help:
	@echo "Available targets:"
	@echo "  make build      - Fetches (if needed) and compiles HSEval and KHE binaries"
	@echo "  make fetch      - Downloads and extracts KHE source without compiling"
	@echo "  make hseval     - Compiles only the HSEval evaluator (bin/hseval)"
	@echo "  make khe        - Compiles the full KHE solver (bin/khe)"
	@echo "  make clean      - Cleans compiled objects and bin directory"
	@echo "  make distclean  - Cleans binaries and deletes the extracted KHE directory"

$(KHE_DIR):
	@echo "==> KHE source directory not found. Fetching $(KHE_VERSION)..."
	@if [ -f "../../downloads/$(KHE_TAR)" ]; then \
		echo "==> Using local archive from downloads/..."; \
		tar -xzf "../../downloads/$(KHE_TAR)" -C "$(CURDIR)"; \
	elif [ -f "../../downloads/khe.tar.gz" ]; then \
		echo "==> Using local archive from downloads/khe.tar.gz..."; \
		tar -xzf "../../downloads/khe.tar.gz" -C "$(CURDIR)"; \
	else \
		echo "==> Downloading $(KHE_URL)..."; \
		if command -v curl >/dev/null 2>&1; then \
			curl -fsSL -o "$(KHE_TAR)" "$(KHE_URL)"; \
		elif command -v wget >/dev/null 2>&1; then \
			wget -q -O "$(KHE_TAR)" "$(KHE_URL)"; \
		elif command -v python3 >/dev/null 2>&1; then \
			python3 -c "import urllib.request; urllib.request.urlretrieve('$(KHE_URL)', '$(KHE_TAR)')"; \
		elif command -v python >/dev/null 2>&1; then \
			python -c "import urllib.request; urllib.request.urlretrieve('$(KHE_URL)', '$(KHE_TAR)')"; \
		else \
			echo "Error: curl, wget, or python required to download KHE" >&2; exit 1; \
		fi; \
		echo "==> Extracting $(KHE_TAR)..."; \
		tar -xzf "$(KHE_TAR)" -C "$(CURDIR)"; \
		rm -f "$(KHE_TAR)"; \
	fi
	@echo "==> KHE source ready in $(KHE_DIR)"

fetch: $(KHE_DIR)

build: $(KHE_DIR) hseval khe
	@echo ""
	@echo "==> Build complete. Executables ready in $(BIN_DIR):"
	@ls -la $(BIN_DIR)

hseval: $(KHE_DIR)
	@mkdir -p $(BIN_DIR)
	$(MAKE) -C $(KHE_DIR)/src_platform
	$(MAKE) -C $(KHE_DIR)/src_hseval FINAL_DIR=$(BIN_DIR)
	@cp -f $(KHE_DIR)/src_hseval/hseval.cgi $(BIN_DIR)/hseval
	@chmod +x $(BIN_DIR)/hseval
	@echo "==> hseval compiled successfully."

khe: $(KHE_DIR)
	@mkdir -p $(BIN_DIR)
	$(MAKE) -C $(KHE_DIR)/src_platform
	$(MAKE) -C $(KHE_DIR)/src_solvers
	$(MAKE) -C $(KHE_DIR)/src_khe FINAL_DIR=$(BIN_DIR)
	@cp -f $(KHE_DIR)/src_khe/khe $(BIN_DIR)/khe
	@chmod +x $(BIN_DIR)/khe
	@echo "==> khe compiled successfully."

clean:
	-if [ -d "$(KHE_DIR)" ]; then \
		$(MAKE) -C $(KHE_DIR)/src_platform clean; \
		$(MAKE) -C $(KHE_DIR)/src_solvers clean; \
		$(MAKE) -C $(KHE_DIR)/src_hseval clean; \
		$(MAKE) -C $(KHE_DIR)/src_khe clean; \
	fi
	-rm -rf $(BIN_DIR)
	@echo "==> Clean complete."

distclean: clean
	-rm -rf "$(KHE_DIR)"
	-rm -f "$(KHE_TAR)"
	@echo "==> Distclean complete. Source directory removed."
