AM_CXXFLAGS += -I$(top_srcdir)

BUILT_SOURCES = bpl_scanner.cc bpl_grammar.cc bpl_grammar.hh

EXTRA_DIST = bpl_scanner.ll bpl_grammar.yy

bpl_grammar.cc: bpl_grammar.yy
	bison -d -o bpl_grammar.cc bpl_grammar.yy

bpl_scanner.cc: bpl_scanner.ll bpl_grammar.cc
	flex bpl_scanner.ll

noinst_HEADERS = \
bpl_grammar.hh \
syntax_tree.h \
generator.h

bin_PROGRAMS = betula

betula_SOURCES = \
main.cpp \
bpl_scanner.cc \
bpl_grammar.cc \
syntax_tree.cpp \
generator.cpp \
cg_cpp.cpp


#betula_LDADD = $(top_builddir)/libbetula/libbetula.la

clean-local:
	-rm -f $(BUILT_SOURCES)

