if(WIN32)
	add_definitions(
		-D_CRT_SECURE_NO_WARNINGS
		-D_WINSOCK_DEPRECATED_NO_WARNINGS
		-DWIN32_LEAN_AND_MEAN
	)
endif()

file(GLOB_RECURSE RAKNET_SOURCES
	"Include/raknet/*.h"
	"Source/*.cpp"
	SAMPRakNet.cpp
	SAMPRakNet.hpp
)

add_library(raknet STATIC ${RAKNET_SOURCES})

set_property(TARGET raknet PROPERTY POSITION_INDEPENDENT_CODE ON)

target_compile_definitions(raknet
	PUBLIC
		-DRAKNET_LEGACY
		-DPACKETS_PER_TICK=3
)
target_include_directories(raknet 
	INTERFACE Include
	PRIVATE Include/raknet
)

if (WIN32)
	target_link_libraries(raknet Ws2_32 OMP-SDK OMP-Network)
else()
	target_link_libraries(raknet OMP-SDK OMP-Network)
endif()
