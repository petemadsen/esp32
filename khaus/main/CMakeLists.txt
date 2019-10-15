#set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_ADD_INCLUDEDIRS .)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

file(GLOB_RECURSE HEADERS "*.h")

set(COMPONENT_SRCS
	${HEADERS}
	main.c
	buttons.c
	my_http.c
	my_lights.c
	my_sensors.c
	my_shutters.c
	my_sleep.c
	tone.c
	lights/lamp.c
	lights/ws2812b.c
	sensors/bmp280.c
	sensors/voltage.c
	sensors/dht12.c
	sound/read_wav.c
	system/my_i2c.c
	system/my_log.c
	system/my_settings.c
	system/ota.c
	system/wifi.c
)

register_component()
