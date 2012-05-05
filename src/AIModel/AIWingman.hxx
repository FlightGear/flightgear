// FGAIWingman - FGAIBllistic-derived class creates an AI Wingman
//
// Written by Vivian Meazza, started February 2008.
// - vivian.meazza at lineone.net
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef _FG_AIWINGMAN_HXX
#define _FG_AIWINGMAN_HXX

#include "AIBallistic.hxx"

#include "AIManager.hxx"
#include "AIBase.hxx"

#include <simgear/sg_inlines.h>


class FGAIWingman : public FGAIBallistic {
public:
    FGAIWingman();
    virtual ~FGAIWingman();

    virtual void readFromScenario(SGPropertyNode* scFileNode);

    bool init(bool search_in_AI_path=false);
    virtual void bind();
    virtual void reinit();
    virtual void update (double dt);

    virtual const char* getTypeString(void) const { return "wingman"; }

private:
    void formateToAC(double dt);
    void Break(double dt);
    void Join(double dt);
    void Run(double dt);

    double getDistanceToOffset() const;
    double getElevToOffset() const;

    double calcAngle(double rangeM, SGGeod pos1, SGGeod pos2);
    double calcDistanceM(SGGeod pos1, SGGeod pos2) const;

    bool   _formate_to_ac;
    bool   _break;
    bool   _join;

    double _break_angle; //degrees relative
    double _coeff_hdg; //dimensionless coefficient
    double _coeff_pch; //dimensionless coefficient
    double _coeff_bnk; //dimensionless coefficient
    double _coeff_spd; //dimensionless coefficient

    SGPropertyNode_ptr user_WoW_node;

    inline void setFormate(bool f);
    inline void setTgtHdg(double hdg);
    inline void setTgtSpd(double spd);
    inline void setBrkHdg(double angle);
    inline void setBrkAng(double angle);
    inline void setCoeffHdg(double h);
    inline void setCoeffPch(double p);
    inline void setCoeffBnk(double r);
    inline void setCoeffSpd(double s);

    inline bool getFormate() const { return _formate_to_ac;}

    inline double getTgtHdg() const { return tgt_heading;}
    inline double getTgtSpd() const { return tgt_speed;}
    inline double getBrkAng() const { return _break_angle;}

    inline SGVec3d getCartInPos(SGGeod in_pos) const;

};

void FGAIWingman::setFormate(bool f) {
    _formate_to_ac = f;
}

void FGAIWingman::setTgtHdg(double h) {
    tgt_heading = h;
}

void FGAIWingman::setTgtSpd(double s) {
    tgt_speed = s;
}

void FGAIWingman::setBrkHdg(double a){
    tgt_heading = hdg + a ;
    SG_NORMALIZE_RANGE(tgt_heading, 0.0, 360.0);
}

void FGAIWingman::setBrkAng(double a){
    _break_angle = a ;
    SG_NORMALIZE_RANGE(_break_angle, -180.0, 180.0);
}

void FGAIWingman::setCoeffHdg(double h){
    _coeff_hdg = h;
}

void FGAIWingman::setCoeffPch(double p){
    _coeff_pch = p;
}

void FGAIWingman::setCoeffBnk(double b){
    _coeff_bnk = b;
}

void FGAIWingman::setCoeffSpd(double s){
    _coeff_spd = s;
}

//bool FGAIWingman::getFormate() const {
//    return _formate_to_ac;
//}

//double FGAIWingman::getTgtHdg() const{
//    return tgt_heading;
//}

//double FGAIWingman::getTgtSpd() const{
//    return tgt_speed;
//}

//double FGAIWingman::getBrkAng() const{
//    return _break_angle;
//}

SGVec3d FGAIWingman::getCartInPos(SGGeod in_pos) const {
    SGVec3d cartPos = SGVec3d::fromGeod(in_pos);
    return cartPos;
}

#endif  // FG_AIWINGMAN_HXX
