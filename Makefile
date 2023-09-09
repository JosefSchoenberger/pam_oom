# main output:
pam_oom.so: $(patsubst %.c,%.o,$(wildcard *.c))

release: pam_oom.so
release: DBGFLAGS?=
release: OPTFLAGS?=-O3

TO_CLEAN=pam_oom.so
EXECUTABLES=

BUILDDIR?=build

WFLAGS?=-Wall -Wextra
DBGFLAGS?=-g
OPTFLAGS?=
DEFINEFLAGS?=
SANITIZERFLAGS?=
EXTRAFLAGS?=

# --------------

MAKEFLAGS+=-Rr

QUIET?=y
ifeq "$(QUIET)" "y"
    Q=@
else
    Q=
endif

ifneq "$(CC)" "default"
    CC=gcc
endif

MAKEDEPS_FLAGS=-MMD -MF $(BUILDDIR)/generated_deps/$(1:$(BUILDDIR)/%.o=%.d)
CCFLAGS=$(WFLAGS) $(DBGFLAGS) $(OPTFLAGS) $(DEFINEFLAGS) $(SANITIZERFLAGS) $(EXTRAFLAGS)

vpath %.o $(BUILDDIR)

$(BUILDDIR):
	$(Q)mkdir -p $@
$(BUILDDIR)/generated_deps:
	$(Q)mkdir -p $@

$(patsubst %.c,$(BUILDDIR)/%.o,$(wildcard *.c)): $(BUILDDIR)/%.o: %.c | $(BUILDDIR)/generated_deps
	@printf "[ %-3s ] %-8s from %s\n" "$(CC)" "$(@:$(BUILDDIR)/%=%)" "$<"
	$(Q)$(CC) -o $@ $< $(CCFLAGS) -c -fPIC $(call MAKEDEPS_FLAGS, $@)

%.so: | $(BUILDDIR)
	@printf "[ %-3s ] %-8s from %s\n" "$(CC)" "$@" "$^"
	$(Q)$(CC) -o $@ $^ -shared

$(EXECUTABLES): %: | $(BUILDDIR)
	@printf "[ %-3s ] %-8s from %s\n" "$(CC)" "$@" "$^"
	$(Q)$(CC) -o $@ $^ $(SANITIZERFLAGS)

.PHONY: clean
clean:
	$(Q)rm -rf $(BUILDDIR)
	$(Q)rm -f $(TO_CLEAN)

.PHONY: install
install: release
	$(Q)install -Dvp -m 0644 -o root -g root -t /lib/security/ pam_oom.so

.PHONY: uninstall
uninstall:
	$(Q)mv /lib/security/pam_oom.so /lib/security/pam_oom.so.bak
#include $(wildcard $(BUILDDIR)/generated_deps/*.d)
include $(shell find '$(BUILDDIR)/generated_deps/' -name '*.d' -type f 2>/dev/null)
