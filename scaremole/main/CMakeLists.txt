set(COMPONENT_ADD_INCLUDEDIRS include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES)
set(COMPONENT_PRIV_REQUIRES)

file(GLOB HEADERS "*.h")

set(COMPONENT_SRCS
	${HEADERS}
	main.c
	wifi.c
	my_http.c
	ota.h
	ota.c
	my_sleep.c
	my_shutters.c
	voltage.c
	scaremole.c
)

register_component()
