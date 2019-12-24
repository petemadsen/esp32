#set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_ADD_INCLUDEDIRS .)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

file(GLOB_RECURSE HEADERS "*.h")

set(COMPONENT_SRCS
	${HEADERS}
	msgeq7.c
	telnet.c
	ws2812b.c
	parser.c
	main.c
	colors.c
	my_http.c
	system/ota.c
	system/my_settings.c
	system/wifi.c
)

register_component()
