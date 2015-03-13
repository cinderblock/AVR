/* 
 * File:   bigTypes.h
 * Author: chtacklind
 *
 * Created on December 14, 2011, 5:18 PM
 */

#ifndef BIGTYPES_H
#define	BIGTYPES_H

#include "basicTypes.h"

typedef union {
 struct {
  u1 A;
  u1 B;
 };
 u2 WORD;
} f2;

typedef union {
 struct {
  u1 A;
  u1 B;
  u1 C;
  u1 D;
 };
 u4 LONG;
} f4;

#endif	/* BIGTYPES_H */

