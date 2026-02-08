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

.PHONY: android android-apk
android:
	$(MAKE) -f Makefile.android android

android-apk:
	@echo "Building Android APK..."
	$(MAKE) -f Makefile.android android
	cd platforms/android && ~/Library/Android/sdk/tools/bin/sdkmanager --install "build-tools;34.0.0" || true
	cd platforms/android && ANDROID_HOME=~/Library/Android/sdk ~/Library/Android/sdk/tools/bin/sdkmanager --list | head -20
	@echo "Use Android Studio to build APK or install Gradle manually"

.PHONY: linux-package linux-package-deploy
linux-package:
	./build-linux-docker.sh

linux-package-deploy: linux-package
	mkdir -p ../../releases
	@mv build/linux/ChipNomad-*-Linux-*.tar.gz ../../releases/ 2>/dev/null && \
		echo "Linux package moved to ../../releases/" || \
		(echo "Error: Package not found in build/linux/"; exit 1)

.PHONY: clean-android
clean-android:
	rm -rf build/android-arm64 build/android-arm32

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
