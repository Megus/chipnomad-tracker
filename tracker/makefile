# Main makefile - delegates to platform-specific makefiles

# Auto-detect platform and delegate
ifeq ($(CROSS_COMPILE),)
ifeq ($(OS),Windows_NT)
# Windows
.PHONY: all
all: windows
else
# Desktop (Linux/macOS)
.PHONY: all
all: desktop
endif
else
# Cross-compile (RG35xx)
.PHONY: all
all: RG35xx
endif

# ============================================================================
# Standard Build Targets: {platform} and {platform}-deploy
# ============================================================================

# Desktop (main development target - works on macOS and Linux)
.PHONY: desktop
desktop:
	$(MAKE) -f Makefile.desktop desktop

# macOS
.PHONY: macOS macOS-deploy
macOS macOS-deploy:
	$(MAKE) -f Makefile.macos $@

# Windows
.PHONY: windows windows-deploy
windows windows-deploy:
	$(MAKE) -f Makefile.windows $@

# Linux
.PHONY: linux linux-deploy
linux linux-deploy:
	$(MAKE) -f Makefile.linux $@

# Android
.PHONY: android android-deploy
android android-deploy:
	$(MAKE) -f Makefile.android $@

# PortMaster
.PHONY: PortMaster PortMaster-deploy
PortMaster PortMaster-deploy:
	$(MAKE) -f Makefile.portmaster $@

# MiyooPorts
.PHONY: MiyooPorts MiyooPorts-deploy
MiyooPorts MiyooPorts-deploy:
	$(MAKE) -f Makefile.miyooports $@

# RG35xx
.PHONY: RG35xx RG35xx-deploy
RG35xx RG35xx-deploy:
	$(MAKE) -f Makefile.rg35xx $@

# ============================================================================
# Utility Targets
# ============================================================================

# Tests
.PHONY: test
test:
	$(MAKE) -f Makefile.test

# Deploy all platforms
.PHONY: deploy-all
deploy-all:
	./deploy-all.sh

# Clean all build artifacts
.PHONY: clean
clean:
	rm -rf build
