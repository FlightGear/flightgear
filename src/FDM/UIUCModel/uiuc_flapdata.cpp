/*flapdata.cpp
Implements the flapping data class
Written by Theresa Robinson
robinst@ecf.toronto.edu
*/

//#ifndef flapdata_cpp
//#define flapdata_cpp
#include "uiuc_flapdata.h"
//#include <fstream>
#include <cassert>

///////////////////////////////////////////////////////////
//Implementation of FlapStruct public methods
//////////////////////////////////////////////////////////

flapStruct::flapStruct(){
        Lift=0;
        Thrust=0;
        Inertia=0;
        Moment=0;
}

flapStruct::flapStruct(const flapStruct &rhs){
        Lift=rhs.getLift();
        Thrust=rhs.getThrust();
        Inertia=rhs.getInertia();
        Moment=rhs.getMoment();
}

flapStruct::flapStruct(double newLift, double newThrust,  double newMoment, double newInertia){
        Lift=newLift;
        Thrust=newThrust;
        Inertia=newInertia;
        Moment=newMoment;
}

double flapStruct::getLift() const{
        return Lift;
}

double flapStruct::getThrust() const{
        return Thrust;
}

double flapStruct::getInertia() const{
        return Inertia;
}

double flapStruct::getMoment() const{
        return Moment;
}


/////////////////////////////////////////////////////////////////
//Implementation of FlapData public methods
////////////////////////////////////////////////////////////////

FlapData::FlapData(){
        liftTable=NULL;
        thrustTable=NULL;
        momentTable=NULL;
        inertiaTable=NULL;

        alphaArray=NULL;
        speedArray=NULL;
        freqArray=NULL;
        phiArray=NULL;

        lastAlphaIndex=0;
        lastSpeedIndex=0;
        lastPhiIndex=0;
        lastFreqIndex=0;
}

//A constructor that takes a file name:
//Opens that file and fills all the arrays from it
//sets the guesses to zero for the speed and halfway
//along the array for the alpha and frequency
//All it does is call init

FlapData::FlapData(const char* filename){
  //  printf("init flapdata\n");
        init(filename);
        lastAlphaIndex=0;
        lastSpeedIndex=0;
        lastPhiIndex=0;
        lastFreqIndex=0;
}

//The destructor:
//Frees all memory associated with this object
FlapData::~FlapData(){
  //  printf("deleting flapdata\n");
  int i, j, k;
  for(i=0;i<alphaLength;i++){
    for(j=0;j<speedLength;j++){
      for(k=0;k<freqLength;k++){
	delete[] liftTable[i][j][k];
	delete[] thrustTable[i][j][k];
	delete[] momentTable[i][j][k];
	delete[] inertiaTable[i][j][k];
      }
      delete[] liftTable[i][j];
      delete[] thrustTable[i][j];
      delete[] momentTable[i][j];
      delete[] inertiaTable[i][j];
    }
    delete[] liftTable[i];
    delete[] thrustTable[i];
    delete[] momentTable[i];
    delete[] inertiaTable[i];
  }
  delete[] liftTable;
  delete[] thrustTable;
  delete[] momentTable;
  delete[] inertiaTable;
  delete[] alphaArray;
  delete[] speedArray;
  delete[] freqArray;
  delete[] phiArray;
}

//An initialization function that does the same thing
//as the second constructor
//returns zero if it was successful
int FlapData::init(const char* filename){

        ifstream* f=new ifstream(filename);     //open file for reading in text (ascii) mode
        if (f==NULL) {  //file open error
                return(1);
        }
        if(readIn(f)){ //read the file, if there's a problem
                return(2);
        }
        delete f;
        return 0;
  //close the file, return the success of the file close
}

//A function that returns the interpolated values
//for all four associated numbers
//given the angle of attack, speed, and flapping frequency
flapStruct FlapData::flapper(double alpha, double speed, double freq, double phi){

        flapStruct results;
        int i,j,k,l;
        double lift,thrust,moment,inertia;
        if(speed<speedArray[0]){
                speed=speedArray[0];
        }
        if(speed>speedArray[speedLength-1]){
                speed=speedArray[speedLength-1];
        }
        if(alpha<alphaArray[0]){
                alpha=alphaArray[0];
        }
        if(alpha>alphaArray[alphaLength-1]){
                alpha=alphaArray[alphaLength-1];
        }
        i=findIndex(alphaArray,alphaLength,alpha,lastAlphaIndex);
        j=findIndex(speedArray,speedLength,speed,lastSpeedIndex);
        k=findIndex(freqArray,freqLength,freq,lastFreqIndex);
        l=findIndex(phiArray,phiLength,phi,lastPhiIndex);
    
        lift=interpolate(liftTable, i, j, k, l, alpha, speed, freq, phi);        
        thrust=interpolate(thrustTable, i, j, k, l, alpha, speed, freq, phi);
        moment=interpolate(momentTable, i, j, k, l, alpha, speed, freq, phi);
        inertia=interpolate(inertiaTable, i, j, k, l, alpha, speed, freq, phi);
        results=flapStruct(lift,thrust,moment,inertia);
        return results;
}

//////////////////////////////////////////////////////////////////
//Implementation of private  FlapData methods
//////////////////////////////////////////////////////////////////

//A function that returns an index i such that
// array[i] < value < array[i+1]
//The function returns -1 if
// (value < array[0]) OR (value > array[n-1])
//(i.e. the value is not within the bounds of the array)
//It performs a linear search starting at guess
int FlapData::findIndex(double array[], double n, double value, int i){

        while(value<array[i]){  //less than the lower end of interval i
                if(i==0){            //if we're at the start of the array
                        return(-1);                     //there's a problem
                }
                i--;                 //otherwise move to the next lower interval
        }
        while(value>array[i+1]){        //more than the higher end of interval i
                if(i==n-1){             //if we're at the end of the array
                        return(-1);                             //there's a problem
                }
                i++;                    //otherwise move to the next higher interval
        }
//        errmsg("In findIndex: array[" << i << "]= " << array[i] << "<=" << value << "<= array[" << (i+1) << "]=" << array[i+1]);
        return(i);
}

//A function that performs a linear interpolation based on the
//eight points surrounding the value required
double FlapData::interpolate(double**** table, int i, int j, int k, int l, double alpha, double speed, double freq, double phi){
//        errmsg("\t\t\t\t\t\t\t\tGetting Values");
        double f0000=table[i][j][k][l];
        double f0001=table[i][j][k][l+1];
        double f0010=table[i][j][k+1][l];
        double f0011=table[i][j][k+1][l+1];
        double f0100=table[i][j+1][k][l];
        double f0101=table[i][j+1][k][l+1];
        double f0110=table[i][j+1][k+1][l];
        double f0111=table[i][j+1][k+1][l+1];
        double f1000=table[i+1][j][k][l];
        double f1001=table[i+1][j][k][l+1];
        double f1010=table[i+1][j][k+1][l];
        double f1011=table[i+1][j][k+1][l+1];
        double f1100=table[i+1][j+1][k][l];
        double f1101=table[i+1][j+1][k][l+1];
        double f1110=table[i+1][j+1][k+1][l];
        double f1111=table[i+1][j+1][k+1][l+1];

  //      errmsg("\t\t\t\t\t\t\t\t1st pass (3)");
    //    errmsg("phi[" << l << "]=" << phiArray[l] << "; phi[" << (l+1) <<"]=" << phiArray[l+1]);
      //  errmsg("Finding " << phi <<endl;
        double f000=interpolate(phiArray[l],f0000,phiArray[l+1],f0001,phi);
        double f001=interpolate(phiArray[l],f0010,phiArray[l+1],f0011,phi);
        double f010=interpolate(phiArray[l],f0100,phiArray[l+1],f0101,phi);
        double f011=interpolate(phiArray[l],f0110,phiArray[l+1],f0111,phi);
        double f100=interpolate(phiArray[l],f1000,phiArray[l+1],f1001,phi);
        double f101=interpolate(phiArray[l],f1010,phiArray[l+1],f1011,phi);
        double f110=interpolate(phiArray[l],f1100,phiArray[l+1],f1101,phi);
        double f111=interpolate(phiArray[l],f1110,phiArray[l+1],f1111,phi);

//        errmsg("\t\t\t\t\t\t\t\t2nd pass (2)");
        double f00=interpolate(freqArray[k],f000,freqArray[k+1],f001,freq);
        double f01=interpolate(freqArray[k],f010,freqArray[k+1],f011,freq);
        double f10=interpolate(freqArray[k],f100,freqArray[k+1],f101,freq);
        double f11=interpolate(freqArray[k],f110,freqArray[k+1],f111,freq);

  //      errmsg("\t\t\t\t\t\t\t\t3rd pass (1)");
        double f0=interpolate(speedArray[j],f00,speedArray[j+1],f01,speed);
        double f1=interpolate(speedArray[j],f10,speedArray[j+1],f11,speed);

    //    errmsg("\t\t\t\t\t\t\t\t4th pass (0)");
        double f=interpolate(alphaArray[i],f0,alphaArray[i+1],f1,alpha);
        return(f);
}

//A function that performs a linear interpolation based
//on the two nearest points
double FlapData::interpolate(double x0, double y0, double x1, double y1, double x){
        double slope,y;
        assert(x1!=x0);
        slope=(y1-y0)/(x1-x0);
        y=y0+slope*(x-x0);
        return y;
}

//A function called by init that reads in the file
//of the correct format and stores it in the arrays and tables
int FlapData::readIn (ifstream* f){

        int i,j,k,l;
        int count=0;
        char numstr[200];

        f->getline(numstr,200);
        sscanf(numstr,"%d,%d,%d,%d",&alphaLength,&speedLength,&freqLength,&phiLength);

	//Check to see if the first line is 0 0 0 0
	//If so, tell user to download data file
	//Quits FlightGear
	if (alphaLength==0 && speedLength==0 && freqLength==0 && phiLength==0)
	  uiuc_warnings_errors(7,"");

        alphaArray=new double[alphaLength];
        speedArray=new double[speedLength];
        freqArray=new double[freqLength];
        phiArray=new double[phiLength];

        for(i=0;i<alphaLength;i++){
                f->get(numstr,20,',');
                sscanf(numstr,"%lf",&alphaArray[i]);
                f->get();
        }
        f->get();
        for(i=0;i<speedLength;i++){
                f->get(numstr,20,',');
                sscanf(numstr,"%lf",&speedArray[i]);
                f->get();
        }
        f->get();
        for(i=0;i<freqLength;i++){
                f->get(numstr,20,',');
                sscanf(numstr,"%lf",&freqArray[i]);
                f->get();
        }
        f->get();
        for(i=0;i<phiLength;i++){
                f->get(numstr,20,',');
                sscanf(numstr,"%lf",&phiArray[i]);
                f->get();
        }
        f->get();
        liftTable=new double***[alphaLength];
        thrustTable=new double***[alphaLength];
        momentTable=new double***[alphaLength];
        inertiaTable=new double***[alphaLength];
        for(i=0;i<alphaLength;i++){
                liftTable[i]=new double**[speedLength];
                thrustTable[i]=new double**[speedLength];
                momentTable[i]=new double**[speedLength];
                inertiaTable[i]=new double**[speedLength];
                for(j=0;j<speedLength;j++){
                        liftTable[i][j]=new double*[freqLength];
                        thrustTable[i][j]=new double*[freqLength];
                        momentTable[i][j]=new double*[freqLength];
                        inertiaTable[i][j]=new double*[freqLength];
                        for(k=0;k<freqLength;k++){
                                 liftTable[i][j][k]=new double[phiLength];
                                 thrustTable[i][j][k]=new double[phiLength];
                                 momentTable[i][j][k]=new double[phiLength];
                                 inertiaTable[i][j][k]=new double[phiLength];
                        }
                }
        }

        for(i=0;i<alphaLength;i++){
                for(j=0;j<speedLength;j++){
                        for(k=0;k<freqLength;k++){
                                for(l=0;l<phiLength;l++){
                                        f->getline(numstr,200);
                                        sscanf(numstr,"%lf %lf %lf %lf",&liftTable[i][j][k][l],&thrustTable[i][j][k][l],&momentTable[i][j][k][l],&inertiaTable[i][j][k][l]);
                                }
                        }
                }
        }
        return 0;
};

//#endif
