.PHONY: all firmware web extension linux clean install-linux

all:
	./scripts/build-all.sh

firmware:
	pio run && pio run -t buildfs

web:
	pio run -t buildfs

extension:
	mkdir -p dist && cd browser-extension && zip -qr ../dist/redbutton-extension.zip . -x '*.pyc' -x '__pycache__/*'

linux:
	mkdir -p dist && tar -czf dist/redbutton-linux.tar.gz linux scripts/install-linux.sh

install-linux:
	sudo ./scripts/install-linux.sh

clean:
	rm -rf dist .pio

