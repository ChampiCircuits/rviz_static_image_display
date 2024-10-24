//
// Created by jonas on 6/30/21.
//

#include "rviz_static_image_display/StaticImageDisplay.hpp"

#include <OgreBillboardSet.h>
#include <OgreMaterialManager.h>
#include <OgreMesh.h>
#include <OgreSceneNode.h>
#include <OgreSubMesh.h>
#include <OgreTechnique.h>
#include <OgreTextureManager.h>
#include <opencv2/opencv.hpp>
#include <rviz_rendering/material_manager.hpp>
#include <ament_index_cpp/get_package_share_directory.hpp>


constexpr auto RESOURCEGROUP_NAME = "rviz_rendering";

namespace rviz_static_image_display::displays {

    StaticImageDisplay::StaticImageDisplay() {

        
        // Properties:
        image_path_property_ = new rviz_common::properties::StringProperty("Image Path", "", "Absolute path and format 'package://my_package/my_image.png' supported.",
                                                                           this, SLOT(updateImage()), this);
        x_offset_property_ = new rviz_common::properties::FloatProperty("X Offset (m)", 0, "Offset from origin",
                                                                            this, SLOT(updateImage()), this);
        y_offset_property_ = new rviz_common::properties::FloatProperty("Y Offset (m)", 0, "Offset from origin",
                                                                            this, SLOT(updateImage()), this);
        rotation_property_ = new rviz_common::properties::FloatProperty("Rotation (deg)", 0, "Rotation of the image in degrees (around origin)",
                                                                            this, SLOT(updateImage()), this);
        resolution_property_ = new rviz_common::properties::FloatProperty("Resolution (px/m)", 1000, "Resolution of the image in pixels per meter (used for scaling)",
                                                                            this, SLOT(updateImage()), this);
        height_property_ = new rviz_common::properties::FloatProperty("Height (m)", 0, "Height of the image on the z axis",
                                                                            this, SLOT(updateImage()), this);
        tf_frame_property_ = new rviz_common::properties::StringProperty("Frame", "map", "",
                                                                            this, SLOT(updateImage()), this);

        static int static_image_display_count = 0;
        static_image_display_count++;
        materialName = "StaticImageMaterial" + std::to_string(static_image_display_count);
        textureName = "StaticImageTexture" + std::to_string(static_image_display_count);
    }


    StaticImageDisplay::~StaticImageDisplay() {
        if (initialized()) {
            scene_manager_->destroyManualObject(imageObject);
        }
        // unsubscribe();
    }


    std::string StaticImageDisplay::parsePath(const std::string &path) {

        // Supported formats:
        // - absolute path
        // - package://package_name/path

        if (path.find("package://") == 0) {
            std::string package_path = path.substr(10);
            std::string package_name = package_path.substr(0, package_path.find("/"));
            std::string package_relative_path = package_path.substr(package_path.find("/") + 1);

            return ament_index_cpp::get_package_share_directory(package_name) + "/" + package_relative_path;
        } 

        return path;

    }


    void StaticImageDisplay::createTextures() {

        texture = Ogre::TextureManager::getSingleton().createManual(
                textureName, RESOURCEGROUP_NAME, Ogre::TEX_TYPE_2D, currentWidth,
                currentHeight, 1, 0, Ogre::PF_BYTE_BGRA, Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);

        material = rviz_rendering::MaterialManager::createMaterialWithNoLighting(materialName);

        auto rpass = material->getTechniques()[0]->getPasses()[0];
        rpass->createTextureUnitState(textureName);
        rpass->setCullingMode(Ogre::CULL_NONE);
        rpass->setEmissive(Ogre::ColourValue::White);
        rpass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);

        // currentHeight = currentStaticImageParam->height;
        // currentWidth = currentStaticImageParam->width;
    }

    void StaticImageDisplay::onInitialize() {
        // _RosTopicDisplay::onInitialize();

        imageObject = scene_manager_->createManualObject();
        imageObject->setDynamic(true);
        scene_node_->attachObject(imageObject);
    }

    void StaticImageDisplay::reset() {
        // _RosTopicDisplay::reset();
        imageObject->clear();
    }

    void StaticImageDisplay::processImage(const std::string &image_path) {

        // Load image using OpenCV
        cv::Mat image = cv::imread(image_path, cv::IMREAD_UNCHANGED);
        if (image.empty()) {
            setStatus(rviz_common::properties::StatusProperty::Error, "Image", QString::fromStdString("Failed to load image: " + image_path));

            return;
        }

        currentWidth = image.cols;
        currentHeight = image.rows;

        Ogre::Vector3 position;
        Ogre::Quaternion orientation;
        builtin_interfaces::msg::Time stamp_0;
        stamp_0.sec = 0;
        stamp_0.nanosec = 0;
        if (!context_->getFrameManager()->getTransform(tf_frame_property_->getString().toStdString(), stamp_0,
                                                       position, orientation)) {
            setMissingTransformToFixedFrame(tf_frame_property_->getString().toStdString());
            return;
        }
        setTransformOk();

        scene_node_->setPosition(position);
        scene_node_->setOrientation(orientation);


        static bool first = true;
        if (first) {
            createTextures();
            first = false;
        }

        imageObject->clear();
        imageObject->estimateVertexCount(4);
        imageObject->begin(material->getName(), Ogre::RenderOperation::OT_TRIANGLE_FAN, "rviz_rendering");

        auto xOffset = x_offset_property_->getFloat();
        auto yOffset = y_offset_property_->getFloat();
        auto height = currentHeight / resolution_property_->getFloat();
        auto width = currentWidth / resolution_property_->getFloat();

        /**
         *        static_image_display-height
         *     2------------------1
         *     |                  |
         *     |                  | static_image_display-width
         *     |                  |
         *     3------------------0
         *     |---------|     I
         *      y-offset ^     I static_image_display-x-offset
         *               |     I
         *   driving dir |     I
         *               |     I
         *             Spatz   I
         */

        // 0
        imageObject->position(xOffset, height - yOffset, height_property_->getFloat());
        imageObject->textureCoord(0, 0);

        // 1
        imageObject->position(xOffset + width, height - yOffset, height_property_->getFloat());
        imageObject->textureCoord(1, 0);

        // 2
        imageObject->position(xOffset + width, -yOffset, height_property_->getFloat());
        imageObject->textureCoord(1, 1);

        // 3
        imageObject->position(xOffset, -yOffset, height_property_->getFloat());
        imageObject->textureCoord(0, 1);
        imageObject->end();

        scene_node_->resetOrientation();
        scene_node_->setOrientation(Ogre::Quaternion(Ogre::Radian(Ogre::Degree(rotation_property_->getFloat())), Ogre::Vector3::UNIT_Z));


        Ogre::HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();

        // Lock the pixel buffer and get a pixel box
        pixelBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD);
        const Ogre::PixelBox &pixelBox = pixelBuffer->getCurrentLock();

        cv::Mat textureMat(currentHeight, currentWidth, CV_8UC4, (void *) pixelBox.data);

        // Convert image to RGBA into textureMat
        cv::cvtColor(image, textureMat, cv::COLOR_BGR2BGRA);

        // Unlock the pixel buffer
        pixelBuffer->unlock();

    }

    void StaticImageDisplay::onEnable() {
        processImage(parsePath(image_path_property_->getString().toStdString()));
    }

    void StaticImageDisplay::onDisable() {
        reset();
    }

    void StaticImageDisplay::updateImage()
    {        
        processImage(parsePath(image_path_property_->getString().toStdString()));
    }

} // namespace rviz_static_image_display::displays

#include <pluginlib/class_list_macros.hpp> // NOLINT
PLUGINLIB_EXPORT_CLASS(rviz_static_image_display::displays::StaticImageDisplay, rviz_common::Display)