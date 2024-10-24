cmake_minimum_required(VERSION 3.8)
project(rviz_static_image_display)

# find dependencies
find_package(ament_cmake REQUIRED)

find_package(rviz_common REQUIRED)
find_package(rviz_rendering REQUIRED)
find_package(rviz_ogre_vendor REQUIRED)

find_package(OpenCV REQUIRED)


set(rviz_static_image_display_headers_to_moc
        include/rviz_static_image_display/StaticImageDisplay.hpp
        )

foreach (header "${rviz_static_image_display_headers_to_moc}")
    qt5_wrap_cpp(rviz_static_image_display_moc_files "${header}")
endforeach ()

set(rviz_static_image_display_source_files
        src/rviz_static_image_display/StaticImageDisplay.cpp
        )

add_library(rviz_static_image_display SHARED
        ${rviz_static_image_display_moc_files}
        ${rviz_static_image_display_source_files}
        )

set_property(TARGET rviz_static_image_display PROPERTY CXX_STANDARD 17)
target_compile_options(rviz_static_image_display PRIVATE -Wall -Wextra -Wpedantic)

target_include_directories(rviz_static_image_display PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${Qt5Widgets_INCLUDE_DIRS}
        ${OpenCV_INCLUDE_DIRS}
        )

target_link_libraries(rviz_static_image_display PUBLIC
        rviz_ogre_vendor::OgreMain
        rviz_ogre_vendor::OgreOverlay
        opencv_core
        opencv_imgproc
        opencv_highgui
        )

# Causes the visibility macros to use dllexport rather than dllimport,
# which is appropriate when building the dll but not consuming it.
target_compile_definitions(rviz_static_image_display PRIVATE "rviz_static_image_display_BUILDING_LIBRARY")

# prevent pluginlib from using boost
target_compile_definitions(rviz_static_image_display PUBLIC "PLUGINLIB__DISABLE_BOOST_FUNCTIONS")

pluginlib_export_plugin_description_file(rviz_common plugins_description.xml)

ament_target_dependencies(rviz_static_image_display
        PUBLIC
        rviz_common
        rviz_rendering
        )

ament_export_include_directories(include)
ament_export_targets(rviz_static_image_display HAS_LIBRARY_TARGET)
ament_export_dependencies(
        rviz_common
        rviz_ogre_vendor
)

install(
        TARGETS rviz_static_image_display
        EXPORT rviz_static_image_display
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)

install(
        DIRECTORY include/
        DESTINATION include
)

ament_package(
        CONFIG_EXTRAS "rviz_static_image_display-extras.cmake"
)
