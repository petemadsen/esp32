set(COMPONENT_ADD_INCLUDEDIRS include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

file(GLOB HEADERS "*.h")

set(COMPONENT_SRCS
	${HEADERS}
	light.c
	dfplayer.c
	main.c
	my_sleep.c
	wifi.c
	buttons.c
	http.c
	shutters.c
	ota.c
	my_settings.c
)

register_component()
