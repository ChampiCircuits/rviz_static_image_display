//
// Created by jonas on 6/30/21.
//

#include "rviz_birdeye_display/BirdeyeDisplay.hpp"

#include <OgreBillboardSet.h>
#include <OgreMaterialManager.h>
#include <OgreMesh.h>
#include <OgreSceneNode.h>
#include <OgreSubMesh.h>
#include <OgreTechnique.h>
#include <OgreTextureManager.h>
#include <opencv2/opencv.hpp>
#include <rviz_rendering/material_manager.hpp>
#include <sensor_msgs/image_encodings.hpp>

constexpr auto RESOURCEGROUP_NAME = "rviz_rendering";

namespace rviz_birdeye_display::displays {

    BirdeyeDisplay::BirdeyeDisplay() {
        
        // Properties:
        image_path_property_ = new rviz_common::properties::StringProperty("Image Path", "/home/andre/Desktop/Vinyle_2025_BETA_V9.png", "Path to the image file",
                                                                           this, SLOT(updateImage()), this);
        x_offset_property_ = new rviz_common::properties::FloatProperty("X Offset", 0, "X Offset",
                                                                            this, SLOT(updateImage()), this);
        y_offset_property_ = new rviz_common::properties::FloatProperty("Y Offset", 0, "Y Offset",
                                                                            this, SLOT(updateImage()), this);
        resolution_property_ = new rviz_common::properties::FloatProperty("Resolution", 1000, "Resolution",
                                                                            this, SLOT(updateImage()), this);


        static int birdeye_count = 0;
        birdeye_count++;
        materialName = "BirdeyeMaterial" + std::to_string(birdeye_count);
        textureName = "BirdeyeTexture" + std::to_string(birdeye_count);
    }

    BirdeyeDisplay::~BirdeyeDisplay() {
        if (initialized()) {
            scene_manager_->destroyManualObject(imageObject);
        }
        // unsubscribe();
    }

    void BirdeyeDisplay::createTextures() {

        texture = Ogre::TextureManager::getSingleton().createManual(
                textureName, RESOURCEGROUP_NAME, Ogre::TEX_TYPE_2D, currentWidth,
                currentHeight, 1, 0, Ogre::PF_BYTE_BGRA, Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);

        material = rviz_rendering::MaterialManager::createMaterialWithNoLighting(materialName);

        auto rpass = material->getTechniques()[0]->getPasses()[0];
        rpass->createTextureUnitState(textureName);
        rpass->setCullingMode(Ogre::CULL_NONE);
        rpass->setEmissive(Ogre::ColourValue::White);
        rpass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);

        // currentHeight = currentBirdeyeParam->height;
        // currentWidth = currentBirdeyeParam->width;
    }

    void BirdeyeDisplay::onInitialize() {
        // _RosTopicDisplay::onInitialize();

        imageObject = scene_manager_->createManualObject();
        imageObject->setDynamic(true);
        scene_node_->attachObject(imageObject);
    }

    void BirdeyeDisplay::reset() {
        // _RosTopicDisplay::reset();
        imageObject->clear();
    }

    void BirdeyeDisplay::processImage(const std::string &image_path) {

        // Load image using OpenCV
        cv::Mat image = cv::imread(image_path, cv::IMREAD_UNCHANGED);
        if (image.empty()) {
            setStatus(rviz_common::properties::StatusProperty::Error, "Image", QString::fromStdString("Failed to load image: " + image_path));

            return;
        }

        currentWidth = image.cols;
        currentHeight = image.rows;

        setStatus(rviz_common::properties::StatusProperty::Ok, "Params", QString("OK"));

        Ogre::Vector3 position;
        Ogre::Quaternion orientation;
        builtin_interfaces::msg::Time stamp_0;
        stamp_0.sec = 0;
        stamp_0.nanosec = 0;
        if (!context_->getFrameManager()->getTransform("map", stamp_0, // TODO get frame from param
                                                       position, orientation)) {
            setMissingTransformToFixedFrame("map");
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
         *        birdeye-height
         *     2------------------1
         *     |                  |
         *     |                  | birdeye-width
         *     |                  |
         *     3------------------0
         *     |---------|     I
         *      y-offset ^     I birdeye-x-offset
         *               |     I
         *   driving dir |     I
         *               |     I
         *             Spatz   I
         */

        // 0
        imageObject->position(xOffset, height - yOffset, 0);
        imageObject->textureCoord(0, 0);

        // 1
        imageObject->position(xOffset + width, height - yOffset, 0);
        imageObject->textureCoord(1, 0);

        // 2
        imageObject->position(xOffset + width, -yOffset, 0);
        imageObject->textureCoord(1, 1);

        // 3
        imageObject->position(xOffset, -yOffset, 0);
        imageObject->textureCoord(0, 1);
        imageObject->end();

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

    void BirdeyeDisplay::onEnable() {
        processImage(image_path_property_->getString().toStdString());
    }

    void BirdeyeDisplay::onDisable() {
        reset();
    }

    void BirdeyeDisplay::updateImage()
    {        
        processImage(image_path_property_->getString().toStdString());
    }

} // namespace rviz_birdeye_display::displays

#include <pluginlib/class_list_macros.hpp> // NOLINT
PLUGINLIB_EXPORT_CLASS(rviz_birdeye_display::displays::BirdeyeDisplay, rviz_common::Display)