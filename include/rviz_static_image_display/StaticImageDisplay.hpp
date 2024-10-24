//
// Created by jonas on 6/30/21.
//

#ifndef VIZ_BIRDEYEDISPLAY_HPP
#define VIZ_BIRDEYEDISPLAY_HPP

#include <OgreHardwarePixelBuffer.h>
#include <rviz_common/message_filter_display.hpp>
#include <rviz_common/properties/float_property.hpp>
#include <rviz_common/properties/string_property.hpp>

#include "rviz_static_image_display/visibility_control.hpp"

namespace rviz_static_image_display::displays {

    class rviz_static_image_display_PUBLIC StaticImageDisplay : public rviz_common::Display {
        Q_OBJECT

      public:
        StaticImageDisplay();
        ~StaticImageDisplay() override;
      

      private Q_SLOTS:
        void updateImage();

      private:
        void onInitialize() override;

        void reset() override;
        void processImage(const std::string &image_path);

        std::string parsePath(const std::string &path);


        Ogre::ManualObject *imageObject = nullptr;
        Ogre::TexturePtr texture;
        Ogre::MaterialPtr material;
        
        // Properties:
        // image_path, x_offset, y_offset, resolution
        rviz_common::properties::StringProperty *image_path_property_;
        rviz_common::properties::FloatProperty *x_offset_property_;
        rviz_common::properties::FloatProperty *y_offset_property_;
        rviz_common::properties::FloatProperty *resolution_property_;
        rviz_common::properties::FloatProperty *rotation_property_;
        rviz_common::properties::FloatProperty *height_property_;
        rviz_common::properties::StringProperty *tf_frame_property_;

        std::string materialName;
        std::string textureName;

        int currentHeight = 0;
        int currentWidth = 0;

        /**
         * Create new texture with current image size if it has changed
         */
        void createTextures();

        void onEnable() override;
        void onDisable() override;
    };

} // namespace rviz_static_image_display::displays


#endif // VIZ_BIRDEYEDISPLAY_HPP
