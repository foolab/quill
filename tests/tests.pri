
QMAKE_CXXFLAGS += -ftest-coverage -fprofile-arcs -fno-elide-constructors
LIBS += -lgcov
#INCLUDEPATH += . ../src

target.path = $$(DESTDIR)/usr/lib/libquill-tests/
INSTALLS += target

QMAKE_CLEAN += *.gcda *.gcno *.gcov *.log *.xml coverage *.o moc_* *.so
QMAKE_DISTCLEAN += *.gcda *.gcno *.gcov *.log *.xml coverage *.o moc_* *.so
