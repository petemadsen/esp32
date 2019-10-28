#set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_ADD_INCLUDEDIRS .)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

file(GLOB_RECURSE HEADERS "*.h")

set(COMPONENT_SRCS
	${HEADERS}
	main.c
	my_http.c
	ota.c
	system/wifi.c
	system/my_settings.c
)

register_component()
