build:
	ino build

test:
	cd spec && make test

clean:
	rm -f .build/run_all
	ino clean

upload: build
	./upload.sh

all: build test upload
