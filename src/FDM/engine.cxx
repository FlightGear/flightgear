// 10520d test program

#include "10520d.hxx"

int main() {
    FGEngine e;

    e.init();

    for ( int i = 0; i < 10000; ++i ) {
	e.set_IAS( 45 );
	e.set_Throttle_Lever_Pos( (double)i / 100.0 );
	e.set_Propeller_Lever_Pos( 100 );
	e.set_Mixture_Lever_Pos( 75 );

	e.update();
	// cout << "Rho = " << e.get_Rho();
	cout << "Throttle = " << i / 100.0;
	cout << "  RPM = " << e.get_RPM();
 	cout << "  Thrust = " << e.get_FGProp1_Thrust() << endl;
   }

    return 0;
}
