
work_dir?=$(PWD)/..
src_dir?=$(work_dir)
bld_dir?=$(work_dir)/bld
install_dir?=$(work_dir)/install
depends_on_dir?=$(TOOL_OUT_DIR)/bin

GDB_TARGET=microblaze-unknown-elf
GDB_PREFIX=$(install_dir)
GDB_SYSROOT_DIR=/sys-root

GDB_CONF_OPTS=--target=$(GDB_TARGET) --prefix=$(GDB_PREFIX) --with-build-sysroot=$(GDB_SYSROOT_DIR)

# configure opts for gdbserver
# static gdbserver probably makes sense
gdbserver_LDFLAGS=-static
GDBSERVER_CONF_OPTS=--target=$(GDB_TARGET) --host=$(GDB_TARGET) --prefix=/usr \
			--with-build-sysroot=/sys-root --sysconfdir=/etc \
			--localstatedir=/var --program-prefix= --without-uiout \
			--disable-tui --disable-gdbtk --without-x \
			--without-included-gettext --without-develop \
			--disable-werror


all: gdb

$(bld_dir)/Makefile: Makefile
	rsync -au $(work_dir)/* $(bld_dir)/.; \
	cd $(bld_dir);			\
	./configure $(GDB_CONF_OPTS)


gdb: $(bld_dir) $(bld_dir)/Makefile
	cd $(bld_dir);		\
	make -j$(HOST_NCPU)


install: $(install_dir) gdb-install
	if [ ! -z "$(PETALINUX_SPEC_BUILT)" ] && [ -f "$(PETALINUX_SPEC_BUILT)" ]; then \
		sed -i "s/GDB_VER=.*/GDB_VER=$(GDB_VER)/g" "$(PETALINUX_SPEC_BUILT)"; \
		grep GDB_VER= "$(PETALINUX_SPEC_BUILT)" 2>&1 1>/dev/null || echo GDB_VER=$(GDB_VER) >> "$(PETALINUX_SPEC_BUILT)"; \
	fi
	cp $(work_dir)/gdb-environment.* $(GDB_PREFIX)/share/

gdb-install: $(install_dir) $(bld_dir)/Makefile
	cd $(bld_dir);		\
	make install

clean:
	rm -rf $(bld_dir) $(install_dir)
	
$(bld_dir):
	mkdir -p $(bld_dir)

$(install_dir):
	mkdir -p $(GDB_PREFIX)/share/; \
	mkdir -p $(install_dir)
