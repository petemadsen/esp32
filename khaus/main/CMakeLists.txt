#set(COMPONENT_ADD_INCLUDEDIRS include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

file(GLOB_RECURSE HEADERS "*.h")

set(COMPONENT_SRCS
	${HEADERS}
	main.c
	buttons.c
	light.c
	my_http.c
	my_i2c.c
	my_sensors.c
	my_settings.c
	my_shutters.c
	my_sleep.c
	ota.c
	tone.c
	read_wav.c
	wifi.c
	sensors/bmp280.c
	sensors/voltage.c
	sensors/dht12.c
)

register_component()
