release {
  DEFINES += QT_NO_DEBUG_OUTPUT
} debug {
  DEFINES -= QT_NO_DEBUG_OUTPUT
}
QMAKE_LFLAGS += -Wl,--as-needed
