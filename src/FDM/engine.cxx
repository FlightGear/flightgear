// 10520d test program

#include "10520d.hxx"

int main() {
    FGEngine e;

    e.init();

    e.set_IAS( 80 );
    e.set_Throttle_Lever_Pos( 50.0 );
    e.set_Propeller_Lever_Pos( 100.0 );
    e.set_Mixture_Lever_Pos( 75 );

    e.update();

    // cout << "Rho = " << e.get_Rho();
    cout << "Throttle = " << 100.0;
    cout << "  RPM = " << e.get_RPM();
    cout << "  Thrust = " << e.get_FGProp1_Thrust() << endl;

    return 0;
}
