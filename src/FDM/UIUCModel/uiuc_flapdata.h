/*flapdata.h
This is the interface definition file for the structure that
holds the flapping data.
Written by Theresa Robinson
robinst@ecf.toronto.edu
*/

#ifndef _FLAPDATA_H
#define _FLAPDATA_H
#include <simgear/compiler.h>

#include <cstdio>
#include <fstream>
#include <sstream>

#include "uiuc_warnings_errors.h"
//#include "uiuc_aircraft.h"

using std::ifstream;
using std::istringstream;

class flapStruct {
private:
        double Lift,Thrust,Inertia,Moment;
public:
        flapStruct();
        flapStruct(const flapStruct &rhs);
        flapStruct(double newLift, double newThrust, double newMoment, double newInertia);
        double getLift() const;
        double getThrust() const;
        double getInertia() const;
        double getMoment() const;
};


class FlapData {

        //class variables
        private:

                //the following are the arrays of increasing
                //data values that were used to generate the lift, thrust
                //pitch and inertial values
                double* alphaArray;        //angle of attack
                double* speedArray;                      //airspeed at the wing
                double* freqArray;         //flapping frequency
                double* phiArray;
                //the following four tables are generated (e.g. by FullWing)
                //using the data in the previous three arrays
                double**** liftTable;            //4D array: holds the lift data
                double**** thrustTable;          //4D array: holds the thrust data
                double**** momentTable;      //4D array: holds the pitching moment data
                double**** inertiaTable;    //4D array: holds the inertia data

                //The values in the tables and arrays are directly related through
                //their indices, in the following way:
                //For alpha=alphaArray[i], speed=speedArray[j] and freq=freqArray[k]
                //phi=phiArray[l]
                //the lift is equal to liftTable[i][j][k][l]
                int alphaLength, speedLength, freqLength, phiLength;
                int lastAlphaIndex, lastSpeedIndex, lastFreqIndex, lastPhiIndex;
                //since we're assuming the angle of attack, velocity, and frequency
                //don't change much between calls to flap, we keep the last indices
                //as a good guess of where to start searching next time

        //public methods
        public:
                //Constructors:
                        //The default constructor:
                        //Just sets the arrays to null and the guesses to zero
                        FlapData();
                        //A constructor that takes a file name:
                        //Opens that file and fills all the arrays from it
                        //sets the guesses to zero for the speed and halfway
                        //along the array for the alpha and frequency
                        FlapData(const char* filename);
                //The destructor:
                //Frees all memory associated with this object
                ~FlapData();
                //An initialization function that does the same thing
                //as the second constructor
                //returns zero if it was successful
                int init(const char* filename);
                //A function that returns the interpolated values
                //for all four associated numbers
                //given the angle of attack, speed, and flapping frequency
                flapStruct flapper(double alpha, double speed, double frequency, double phi);
        //private methods
        private:
                //A function that returns an index i such that
                // array[i] < value < array[i+1]
                //The function returns -1 if
                // (value < array[0]) OR (value > array[n-1])
                //(i.e. the value is not within the bounds of the array)
                //It performs a linear search starting at LastIndex
                int findIndex(double array[], double n, double value, int LastIndex);
                //A function that performs a linear interpolation based on the
                //eight points surrounding the value required
                double interpolate(double**** table, int alphaIndex, int speedIndex, int freqIndex, int phiIndex, double alpha, double speed, double freq, double phi2);
                //A function that performs a linear interpolation based on the two nearest points
                double interpolate(double x1, double y1, double x2, double y2, double x);
                //A function called by init that reads in the file
                //of the correct format and stores it in the arrays and tables
                int readIn(ifstream* f);
};

#endif
