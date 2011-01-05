#ifndef _FG_ATIS_LEXICON_HXX
#define _FG_ATIS_LEXICON_HXX

#include <string>

// NOTE:  This file serves as a database.
// It is read by some utility programs that synthesize
// the library of spoken words.

#define Q(word) const std::string word(#word);

namespace lex {
Q(Airport)
Q(Airfield)
Q(Airbase)
Q(Junior)
Q(Celsius)
Q(wind)
Q(zulu)
Q(zulu_weather)
Q(Automated_weather_observation)
Q(weather)
Q(airport_information)
Q(International)
Q(Regional)
Q(County)
Q(Municipal)
Q(Memorial)
Q(Field)
Q(Air_Force_Base)
Q(Army_Air_Field)
Q(Marine_Corps_Air_Station)
Q(light_and_variable)
Q(at)
Q(thousand)
Q(hundred)
Q(zero)
Q(Temperature)
Q(clear)
Q(isolated)
Q(few)
Q(scattered)
Q(broken)
Q(overcast)
Q(thin)
Q(Sky_condition)
Q(Sky)
Q(Ceiling)
Q(minus)
Q(dewpoint)
Q(Visibility)
Q(less_than_one_quarter)
Q(one_quarter)
Q(one_half)
Q(three_quarters)
Q(one_and_one_half)
Q(Altimeter)
Q(QNH)
Q(millibars)
Q(Landing_and_departing_runway)
Q(On_initial_contact_advise_you_have_information)
Q(This_is)
Q(left)
Q(right)
Q(center)
}

#undef Q
#endif // _FG_ATIS_LEXICON_HXX
