#set(COMPONENT_ADD_INCLUDEDIRS include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS
	main.c
	i2c.c
	ina219.c
)

register_component()
