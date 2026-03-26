# ====================================================================
#  MakefileによるCMakeビルド手順の簡略化
#
#  CMakeのビルドプロセスを簡略化するためのラッパー．
#  複数のボード（Pico、Pico2など）に対応するため `make` のみを実行すると選択メニューが表示される．
#
#  `make pico` など直接ボードを指定すると，そのボード用のファームウェアのみビルドされる．
#  `make all` の場合は全てのボード用のファームウェアがビルドされる．
# ====================================================================

# Makefileで定義されるコマンド一覧（非ファイル）
.PHONY: default all clean help pico pico2 setup

# make コマンドのデフォルトターゲット
.DEFAULT_GOAL := default

# ボード選択メニュー（選択後に `make <board>` が呼び出される）
default:
	@echo -n "1) pico  2) pico2 : "; \
	read choice; \
	case $$choice in \
		1) $(MAKE) pico;; \
		2) $(MAKE) pico2;; \
		*) echo "Invalid selection" >&2; exit 1;; \
	esac

# 全ボードビルド
all: pico pico2

# ボードターゲット（CMake設定済みのビルドディレクトリに依存）
pico pico2: %: build/%/Makefile
	@$(MAKE) --no-print-directory -C build/$*

# CMakeのビルド設定ファイル（Makefile）の生成ルール
#
# CMakeLists.txt更新時のみ実行される．
build/%/Makefile: CMakeLists.txt
	@cmake -S . -B build/$* -DPICO_BOARD=$*

# ビルドツールのセットアップ
#
# 下記を生成する．
# - mrubyコンパイラ（mrbc）
# - mruby/cの自動生成ヘッダ
setup:
	@$(MAKE) --no-print-directory -C components/mruby
	@$(MAKE) --no-print-directory -C components/mrubyc/src autogen

# ビルド成果物の削除
clean:
	@rm -rf build/*

# 利用可能なコマンドの表示
help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "Available targets:"
	@echo "  (default)   Select board interactively."
	@echo "  all         Build for all boards."
	@echo "  pico        Build for Pico."
	@echo "  pico2       Build for Pico 2."
	@echo "  setup       Build tools (run once after clone)."
	@echo "  clean       Remove all build artifacts."
	@echo "  help        Show this help message."
