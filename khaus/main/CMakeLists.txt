set(COMPONENT_ADD_INCLUDEDIRS include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

file(GLOB HEADERS "*.h")

set(COMPONENT_SRCS
	${HEADERS}
	main.c
	light.c
	my_sleep.c
	wifi.c
	buttons.c
	my_http.c
	my_shutters.c
	ota.c
	my_settings.c
	tone.c
	my_i2c.c
	bmp280.c
	voltage.c
)

register_component()
