/* a quick ls_model.h */


#ifndef _LS_MODEL_H
#define _LS_MODEL_H

typedef enum {
  NAVION,
  C172,
  CHEROKEE,
  BASIC,
  UIUC
} Model;

extern Model current_model;


void ls_model( SCALAR dt, int Initialize );


#endif /* _LS_MODEL_H */
