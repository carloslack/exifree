CC := $(shell which gcc)
FIND := $(shell which find)
MD5 := $(shell which md5sum)
LD_LIBRARY_PATH := $(shell pwd)
EXIFREE := $(shell pwd)/exifree
HASHFILE := $(shell pwd)/test-hashes.txt
HEXFILE := $(shell pwd)/md5-01-sct.txt
PWD := $(shell pwd)

SRCS := exifree.c \
		fs.c \
		buffer.c \
		hexdump.c

define run_exifree_on_targets
	cd $(1) && \
		ls *.jpg| while read x ; do \
				$(EXIFREE) -d . -f $$x >/dev/null && \
				md5=$$($(MD5) wiped_$$x |cut -d " " -f1) && \
				match=$$(grep $$x $(2)| cut -d " " -f1);\
				if [ $$md5 != $$match ]; then \
					echo ;	echo "!!!!!!!!!!!!! Fail hash test for $(1)/$$x :~("; echo; \
				fi; \
		done
endef

define run_exifree_on_targets_sct
	cd $(1) && \
	$(EXIFREE) -f $(3) -s -d . >/dev/null && cd - && \
	hexdump -C $(1)/$(2)/* > $(1)/$(2)/wiped_diff.diff && \
		diff $(HEXFILE) $(1)/$(2)/wiped_diff.diff ; \
		if [ $$? != 0 ]; then \
			echo ;	echo "!!!!!!!!!!!!! Fail hex test for $(1)/$(2) files :~("; echo; \
		fi;
endef

all:	
	$(CC) $(SRCS) -Wall -Werror -o $(EXIFREE)

test: clean all
	$(call run_exifree_on_targets, exif-samples/jpg, $(HASHFILE))
	$(call run_exifree_on_targets, exif-samples/jpg/tests, $(HASHFILE))
	$(call run_exifree_on_targets, exif-samples/jpg/mobile, $(HASHFILE))
	$(call run_exifree_on_targets, exif-samples/jpg/gps, $(HASHFILE))
	$(call run_exifree_on_targets, exif-samples/jpg/hdr, $(HASHFILE))
	$(call run_exifree_on_targets_sct, exif-samples/jpg/gps,wiped_DSCN0010.jpg-bin,DSCN0010.jpg)

install: all
	mkdir -p /opt/exifree/bin
	cp -v $(EXIFREE) /opt/exifree/bin/$(EXIFREE)

clean:
	rm -f exifree
	rm -rf exif-samples/jpg/gps/wiped_DSCN0010.jpg-bin
	$(FIND) exif-samples/jpg/ -name "wiped_*" -exec rm -f {} \;
