# ====================================================================
#  MakefileによるCMakeビルド手順の簡略化
#
#  CMakeのビルドプロセスを簡略化するためのラッパー．
#  プロジェクトのルートディレクトリで `make` を実行すると，ビルドディレクトリの作成，CMakeの実行，コンパイルが自動的に行われる．
# ====================================================================

# Makefileで定義されるコマンド一覧（非ファイル）
.PHONY: all clean configure mrbc help

# make コマンドのデフォルトで実行されるターゲット指定
.DEFAULT_GOAL := all

# プロジェクトのビルド
all: build/Makefile
	@make --no-print-directory -C build

# CMakeのビルド設定ファイル（Makefile）の生成ルール
#
# CMakeLists.txt更新時のみ実行される．
build/Makefile: CMakeLists.txt
	@cmake -S . -B build

# ビルド成果物の削除
clean:
	@rm -rf build/*

mrbc:
	@mkdir -p build
	@mrbc -o build/master.mrbc src/master.rb

# 利用可能なコマンドの表示
help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "Available targets:"
	@echo "  all         (Default) Configure and build the project."
	@echo "  clean       Remove all build artifacts."
	@echo "  help        Show this help message."
