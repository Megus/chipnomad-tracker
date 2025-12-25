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

.PHONY: linux-package linux-package-deploy
linux-package:
	./build-linux-docker.sh

linux-package-deploy: linux-package
	mkdir -p ../../releases
	$(eval VERSION := $(shell grep 'appVersion.*=' src/version.c | cut -d'"' -f2))
	@if [ -f build/linux/ChipNomad-$(VERSION)-Linux-x86_64.tar.gz ]; then \
		mv build/linux/ChipNomad-$(VERSION)-Linux-x86_64.tar.gz ../../releases/ChipNomad-$$(date +%Y-%m-%d)-$(VERSION)-Linux-x86_64.tar.gz; \
		echo "Linux package created: ../../releases/ChipNomad-$$(date +%Y-%m-%d)-$(VERSION)-Linux-x86_64.tar.gz"; \
	else \
		echo "Error: Package not found at build/linux/ChipNomad-$(VERSION)-Linux-x86_64.tar.gz"; \
		exit 1; \
	fi

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

clean-linux:
	rm -rf build/linux
