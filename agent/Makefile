BUILD_DIR := cmake-build-debug
#BUILD_DIR := build
GENERATOR := Unix Makefiles
TARGET    := agent_server
JOBS      := $(shell nproc)

.PHONY: all configure build proto run clean distclean

all: build

# 只在 build 目錄還沒設定過時才執行 configure
$(BUILD_DIR)/Makefile:
	#cmake -S . -B $(BUILD_DIR) -G "$(GENERATOR)"
	cmake -S . -B $(BUILD_DIR) -G "$(GENERATOR)" -DCMAKE_BUILD_TYPE=Debug
configure: $(BUILD_DIR)/Makefile

build: configure
	cmake --build $(BUILD_DIR) -j$(JOBS)

# 只重新產生 proto / grpc 檔案
proto: configure
	cmake --build $(BUILD_DIR) --target generate_proto

run: build
	./$(BUILD_DIR)/$(TARGET)

# 清除編譯產物
clean:
	-cmake --build $(BUILD_DIR) --target clean

# 連 build 目錄與生成的 proto 檔一併刪除
distclean:
	rm -rf $(BUILD_DIR)
	rm -f src/proto/*.pb.h src/proto/*.pb.cc