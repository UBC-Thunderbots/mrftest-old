#include "Filter.h"

Filter::Filter(double B_0, double B_1, double A_1):
B_0(B_0),B_1(B_1),A_1(A_1),B_2(0),A_2(0){
  full=0;                        
  delayed[0]=0;
  delayed[1]=0;
}

Filter::Filter(double B_0, double B_1, double A_1,double B_2,double A_2):
B_0(B_0),B_1(B_1),A_1(A_1),B_2(B_2),A_2(A_2){
  full=1;
  delayed[0]=0;
  delayed[1]=0;
}

void Filter::nuke(){
  delayed[0]=delayed[1]=0;
}

double Filter::process(double input) {
  static double current;
  static double output; 
  if(full){
    current=input-A_1*delayed[0]-A_2*delayed[1];
    output=B_0*current+B_1*delayed[0]+B_2*delayed[1];
    delayed[1]=delayed[0];
    delayed[0]=current;
  }
  else{
    current=input-A_1*delayed[0];
    output=B_0*current+B_1*delayed[0];
    delayed[0]=current;
  }
  return output;
}
