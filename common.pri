contains( debug, yes ) {
     message( "Configuring for debug build ..." )
     CONFIG += debug
} else {
     message( "Configuring for release build ..." )
     CONFIG += release
     DEFINES += QT_NO_DEBUG_OUTPUT
}
