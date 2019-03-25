#include "ImageParser.hpp"
#include "XlfResources.hpp"

const MediaGeometry::ScaleType DEFAULT_SCALE_TYPE = MediaGeometry::ScaleType::Scaled;
const MediaGeometry::Align DEFAULT_ALIGN = MediaGeometry::Align::Center;
const MediaGeometry::Valign DEFAULT_VALIGN = MediaGeometry::Valign::Middle;

std::istream& operator >>(std::istream& in, MediaGeometry::ScaleType& scaleType)
{
    std::string temp;
    in >> temp;

    if(temp == ResourcesXlf::Media::Scaled)
        scaleType = MediaGeometry::ScaleType::Scaled;
    else if(temp == ResourcesXlf::Media::Stretch)
        scaleType = MediaGeometry::ScaleType::Stretch;

    return in;
}

std::istream& operator >>(std::istream& in, MediaGeometry::Align& align)
{
    std::string temp;
    in >> temp;

    if(temp == ResourcesXlf::Media::LeftAlign)
        align = MediaGeometry::Align::Left;
    else if(temp == ResourcesXlf::Media::CenterAlign)
        align = MediaGeometry::Align::Center;
    else if(temp == ResourcesXlf::Media::RightAlign)
        align = MediaGeometry::Align::Right;

    return in;
}

std::istream& operator >>(std::istream& in, MediaGeometry::Valign& valign)
{
    std::string temp;
    in >> temp;

    if(temp == ResourcesXlf::Media::TopValign)
        valign = MediaGeometry::Valign::Top;
    else if(temp == ResourcesXlf::Media::MiddleValign)
        valign = MediaGeometry::Valign::Middle;
    else if(temp == ResourcesXlf::Media::BottomValign)
        valign = MediaGeometry::Valign::Bottom;

    return in;
}

ImageParser::ImageParser(const xml_node& node) :
    MediaParser(node)
{

}

ImageOptions ImageParser::parse()
{
    auto options = baseOptions();
    auto scaleType = node().get<MediaGeometry::ScaleType>(ResourcesXlf::option(ResourcesXlf::Media::ScaleType), DEFAULT_SCALE_TYPE);
    auto align = node().get<MediaGeometry::Align>(ResourcesXlf::option(ResourcesXlf::Media::Align), DEFAULT_ALIGN);
    auto valign = node().get<MediaGeometry::Valign>(ResourcesXlf::option(ResourcesXlf::Media::Valign), DEFAULT_VALIGN);

    return ImageOptions{options, scaleType, align, valign};
}

