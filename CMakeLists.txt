cmake_minimum_required(VERSION 3.17)
project(TransportCatalogue)

set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS transport_catalogue.proto svg.proto map_renderer.proto transport_router.proto graph.proto)
set(TRANSPORT_CATALOGUE_FILES transport_catalogue main.cpp graph.h ranges.h router.h transport_router.cpp transport_router.h json_builder.cpp json_builder.h geo.h transport_catalogue.h transport_catalogue.cpp domain.cpp domain.h json.cpp json.h json_reader.cpp json_reader.h map_renderer.cpp map_renderer.h request_handler.cpp request_handler.h svg.h svg.cpp serialization.h serialization.cpp)
add_compile_options(-O3 -Wall -Wextra  -march=native -mtune=native)
add_executable(transport_catalogue ${TRANSPORT_CATALOGUE_FILES} ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue PRIVATE -ltbb -lpthread "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" Threads::Threads)