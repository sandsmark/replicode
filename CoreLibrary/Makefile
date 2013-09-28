############# Names and dirs #############

BINLIBNAME = CoreLibrary

BINDIR = ../bin
SRCDIR = .
BUILDDIR = build

INCLUDEDIRS = .

LIBS = 

############# Include system stuff #############

include ../Makefile.sys

############# Files to compile #############

CPPFILES = base.cpp utils.cpp xml_parser.cpp

############# Setup dirs #############

BINLIBRARY = $(BINDESTDIR)/lib$(BINLIBNAME).so

SRCFILES = $(patsubst %.cpp,$(SRCDIR)/%.cpp,$(CPPFILES))
OBJFILES = $(patsubst %.cpp,$(BUILDDIR)/%.o,$(CPPFILES))

INCLUDES = $(patsubst %,-I% ,$(INCLUDEDIRS))
LIBS = $(subst /arch/,/$(ARCH)/,$(LIBRARIES))

RESULT := $(shell if [ ! -d $(BINDIR) ]; then mkdir $(BINDIR); fi)
RESULT := $(shell if [ ! -d $(BINDESTDIR) ]; then mkdir $(BINDESTDIR); fi)
RESULT := $(shell if [ ! -d $(BUILDDIR) ]; then mkdir $(BUILDDIR); fi)

############# Overall commands #############

all : $(BINLIBRARY)
	@echo $(BINLIBRARY) ready

debug: all

profile: all

############# Dependencies #############

$(BUILDDIR)/%.d: $(SRCDIR)/%.cpp
	@echo Making dependencies for $(<F)...
	@$(SHELL) -ec '$(CC) -MM $(INCLUDES) -D $(SYSTEM) -std=c++0x $< \
		| sed '\''s/\($(*F)\)\.o[ :]*/\1.o $(@F) : /g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),totalclean)
  -include $(OBJFILES:.o=.d)
endif
endif

############# Compiling #############

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@echo Compiling $(<F)
	@$(CC) $(DEBUG) $(INCLUDES) $(STANDARD) -std=c++0x $(LIBCOPT) $(NOTLINK) $(OUTPUT_OPTION) $<

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@echo Compiling $(<F)
	@gcc $(DEBUG) $(INCLUDES) $(LIBCOPT) -Wall -w -Wstrict-prototypes -Wmissing-prototypes -O2 -o $@ -c $<

############# Linking #############

$(BINLIBRARY): $(OBJFILES)
	@echo Linking $(BINLIBRARY) $(SYSTEM)...
	@$(CC) $(DEBUG) $(STANDARD) $(LIBOPT) $(OBJFILES) $(LIBPATHS) -o $(BINLIBRARY) $(LIBS) $(OTHERLIBS)

############# Cleaning #############

clean:
	@echo Removing build and binary files
	@$(RMALL) $(BUILDDIR)
	@$(RMALL) $(BINLIBRARY)

totalclean : clean
	@echo Removing Visual Studio temporary files
	@$(RMALL) *.suo *.ncb

############# Additional commands #############

rebuild : clean all
