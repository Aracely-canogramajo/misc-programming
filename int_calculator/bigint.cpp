// $Id: bigint.cpp,v 1.2 2020-01-06 13:39:55-08 - - $

#include <cstdlib>
#include <exception>
#include <stack>
#include <stdexcept>
using namespace std;

#include "bigint.h"
#include "debug.h"
#include "relops.h"

bigint::bigint (long that): uvalue (that), is_negative (that < 0) {
   DEBUGF ('~', this << " -> " << uvalue)
}

bigint::bigint (const ubigint& uvalue_, bool is_negative_):
                uvalue(uvalue_), is_negative(is_negative_) {
}

bigint::bigint (const string& that) {
   is_negative = that.size() > 0 and that[0] == '_';
   uvalue = ubigint (that.substr (is_negative ? 1 : 0));
}

bigint bigint::operator+ () const {
   return *this;
}

bigint bigint::operator- () const {
   return {uvalue, not is_negative};
}

bigint bigint::operator+ (const bigint& that) const {
   bigint result;
   if(is_negative == that.is_negative){ //same sign
   if(uvalue > that.uvalue){
   result = uvalue + that.uvalue;
   }else{
    result = that.uvalue + uvalue;
   }
    result.is_negative = is_negative; //save same sign
   }else{ //different sign
    if(uvalue > that.uvalue){
       result = uvalue - that.uvalue;
       result.is_negative = is_negative;
       }else if(uvalue < that.uvalue){
       result = that.uvalue - uvalue;
       result.is_negative = that.is_negative;
     }else{ //same value
       result = uvalue - that.uvalue; //will return 0
       result.is_negative = false; //0 is positive
     }                                                      
   }
   return result;
}

bigint bigint::operator- (const bigint& that) const {
  bigint result;
  if(is_negative == true and that.is_negative== true){ //both neg
    if(uvalue > that.uvalue){
      result = uvalue - that.uvalue; 
      result.is_negative = true; //-8- -2 = -6
    }else if(uvalue < that.uvalue){
      result = that.uvalue - uvalue;
      result.is_negative = false; //-2- -8 = 6
    }else{ //same num
      result = uvalue - that.uvalue;
      result.is_negative = false; //sign of 0 is pos
    }
  }
  else if(is_negative == false and that.is_negative==false){//both pos
    if(uvalue > that.uvalue){
      result = uvalue - that.uvalue;
      result.is_negative = false; // 8-2 = +6
    }else if( uvalue < that.uvalue){
      result = that.uvalue - uvalue;
      result.is_negative = true; //2-8 = -6
    }else{ //both same nu
      result = uvalue - that.uvalue;
      result = true; 
    }
  }else{ //different signs
    if(uvalue > that.uvalue){
      result = uvalue - that.uvalue;
    }else if(uvalue < that.uvalue){
      result = that.uvalue - uvalue;
    }else{
      if(uvalue > that.uvalue){
        result = uvalue + that.uvalue;
      }else{
        result = that.uvalue + uvalue;
       }
    }
    result.is_negative = is_negative;
  }
  return result;
}


bigint bigint::operator* (const bigint& that) const {
   bigint result;
   result = uvalue * that.uvalue;
   if(is_negative != that.is_negative){ //diff signs
   result.is_negative = true; //set negative
   }else{
     result.is_negative = false;
   }
   return result;
}

bigint bigint::operator/ (const bigint& that) const {
   bigint result;
   result = uvalue / that.uvalue;
   if(is_negative != that.is_negative){ //diff signs
    result.is_negative = true; //set negative
   }else{
     result.is_negative = false;
   }
   return result;
}

bigint bigint::operator% (const bigint& that) const {
   bigint result = uvalue % that.uvalue;
   return result;
}

bool bigint::operator== (const bigint& that) const {
   return is_negative == that.is_negative and uvalue == that.uvalue;
}

bool bigint::operator< (const bigint& that) const {
   if (is_negative != that.is_negative) return is_negative;
   return is_negative ? uvalue > that.uvalue
                      : uvalue < that.uvalue;
}

ostream& operator<< (ostream& out, const bigint& that) {
  if(that.is_negative){
    out << "-";
  }
  return out << that.uvalue;
}

