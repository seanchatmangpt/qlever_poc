.PHONY: help build build-dev build-all test clean install

help:
	@echo "QLever WASM Build System"
	@echo "======================="
	@echo ""
	@echo "Available targets:"
	@echo "  make install       - Install dependencies"
	@echo "  make build         - Build optimized WASM module"
	@echo "  make build-dev     - Build development WASM module"
	@echo "  make build-browser - Build browser bundle"
	@echo "  make build-node    - Build Node.js bundle"
	@echo "  make build-all     - Build all targets"
	@echo "  make test          - Run tests"
	@echo "  make test-node     - Run Node.js tests"
	@echo "  make clean         - Clean build artifacts"
	@echo "  make help          - Show this help message"

install:
	npm install

build:
	npm run build

build-dev:
	npm run build:dev

build-browser:
	npm run build:browser

build-node:
	npm run build:node

build-all:
	npm run build:all

test:
	npm test

test-node:
	npm run test:node

clean:
	npm run clean
	rm -rf node_modules

.PHONY: watch
watch:
	cargo watch -i pkg -i dist -i node_modules -x "wasm-pack build --dev"
