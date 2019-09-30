#set(COMPONENT_ADD_INCLUDEDIRS include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS
        common.h
        msgeq7.c
        msgeq7.h
        telnet.c
        telnet.h
        ws2812b.c
        ws2812b.h
        parser.c
        parser.h
        main.c
	colors.c
        colors.h
        my_http.c
        my_http.h
        wifi.c
        wifi.h
        ota.h
        ota.c
)

register_component()
