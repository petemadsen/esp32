#set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_ADD_INCLUDEDIRS .)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

file(GLOB_RECURSE HEADERS "*.h")

set(COMPONENT_SRCS
	${HEADERS}
	main.c
	wifi.c
	my_http.c
	oled.c
	ota.c
	my_sleep.c
	my_shutters.c
	voltage.c
	scaremole.c
	ds18b20.c
	system/my_settings.c
	system/my_i2c.c
	system/one_wire.c
)

register_component()
