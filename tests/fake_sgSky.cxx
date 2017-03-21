#include "fake_sgSky.hxx"

void SGSky::add_cloud_layer(SGCloudLayer *layer)
{
    _cloudLayers.push_back(layer);
}

SGCloudLayer *SGSky::get_cloud_layer(int i)
{
    return _cloudLayers.at(i);
}

int SGSky::get_cloud_layer_count() const
{
    return _cloudLayers.size();
}

void SGSky::set_clouds_enabled(bool enabled)
{
    _3dcloudsEnabled = enabled;
}

const std::string &SGCloudLayer::getCoverageString() const
{
    return std::string();
}

const std::string &SGCloudLayer::getCoverageString(SGCloudLayer::Coverage coverage)
{
    return std::string();
}

void SGCloudLayer::setCoverageString(const std::string &coverage)
{

}

void SGCloudLayer::set_enable3dClouds(bool enable)
{

}
