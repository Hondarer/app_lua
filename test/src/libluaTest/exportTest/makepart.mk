LIBS += lua

ifdef PLATFORM_LINUX
    LIBS += m dl
endif
