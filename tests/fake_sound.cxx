#include <simgear/sound/soundmgr.hxx>

class SGSoundMgr::SoundManagerPrivate
{
public:
};

SGSoundMgr::SGSoundMgr()
{

}

SGSoundMgr::~SGSoundMgr()
{

}

void SGSoundMgr::init()
{

}

void SGSoundMgr::stop()
{


}

void SGSoundMgr::suspend()
{

}

void SGSoundMgr::resume()
{

}

void SGSoundMgr::update(double dt)
{
}

void SGSoundMgr::reinit()
{

}

bool SGSoundMgr::load(const std::string &samplepath, void **data, int *format, size_t *size, int *freq, int *block)
{
	return false;
}

std::vector<std::string> SGSoundMgr::get_available_devices()
{
    std::vector<std::string> result;
    return result;
}
