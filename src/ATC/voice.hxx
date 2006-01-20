// atc_voice.hxx
#include <simgear/constants.h>
#include <simgear/compiler.h>

#include STL_STRING
SG_USING_STD(string);

class FGVoice 
{
public:
		FGVoice();
		~FGVoice();
			
		bool send_transcript( string trans, string refname, short repeat );
		
};
extern FGVoice *p_Voice;

