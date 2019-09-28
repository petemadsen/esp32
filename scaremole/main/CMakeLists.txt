set(COMPONENT_ADD_INCLUDEDIRS include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES)
set(COMPONENT_PRIV_REQUIRES)

set(COMPONENT_SRCS
	main.c
	common.h
	wifi.h
	wifi.c
	my_http.h
	my_http.c
	ota.h
	ota.c
	my_sleep.h
	my_sleep.c
	shutters.h
	shutters.c
	voltage.h
	voltage.c
	scaremole.h
	scaremole.c
)

register_component()
