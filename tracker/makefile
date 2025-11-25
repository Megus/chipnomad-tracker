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

# Explicit platform targets
.PHONY: desktop
desktop:
	$(MAKE) -f Makefile.desktop desktop

.PHONY: windows
windows:
	$(MAKE) -f Makefile.windows windows

.PHONY: RG35xx RG35xx-deploy
RG35xx RG35xx-deploy:
	$(MAKE) -f Makefile.rg35xx $@

.PHONY: PortMaster PortMaster-deploy
PortMaster PortMaster-deploy:
	$(MAKE) -f Makefile.portmaster $@

.PHONY: macOS macOS-deploy
macOS macOS-deploy:
	$(MAKE) -f Makefile.macos $@

.PHONY: windows-deploy
windows-deploy:
	$(MAKE) -f Makefile.windows windows-deploy

.PHONY: deploy-all
deploy-all:
	./deploy-all.sh

.PHONY: clean
clean:
	rm -rf build

.PHONY: clean-desktop clean-windows clean-rg35xx clean-portmaster
clean-desktop:
	rm -rf build/desktop

clean-windows:
	rm -rf build/windows

clean-rg35xx:
	rm -rf build/rg35xx

clean-portmaster:
	rm -rf build/portmaster

clean-macOS:
	rm -rf build/macos
