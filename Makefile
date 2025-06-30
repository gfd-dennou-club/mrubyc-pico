# ====================================================================
#  MakefileによるCMakeビルド手順の簡略化。
#
#  このMakefileは、CMakeのビルドプロセスを簡略化するためのラッパー。
#  プロジェクトのルートディレクトリで `make` を実行して、ビルドディレクトリの作成、CMakeの実行、コンパイルが自動的に行われる。
# ====================================================================

# .PHONYは、実際のファイル名ではなくコマンド名であることをmakeに伝える。
.PHONY: all clean configure mrbc help

# デフォルトのゴールを 'all' に設定する。
.DEFAULT_GOAL := all

# プロジェクトのビルド。
all: build/Makefile
	@make --no-print-directory -C build

# CMakeのビルド設定ファイル（Makefile）を生成するルール。
# このファイルが存在しないか、CMakeLists.txtが更新された場合にのみ実行される。
build/Makefile: CMakeLists.txt
	@cmake -S . -B build

# ビルド成果物をすべて削除。
clean:
	@rm -rf build

mrbc:
	@mkdir -p build
	@mrbc -o build/master.mrbc src/master.rb

# 利用可能なコマンドの表示。
help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "Available targets:"
	@echo "  all         (Default) Configure and build the project."
	@echo "  clean       Remove all build artifacts."
	@echo "  help        Show this help message."
