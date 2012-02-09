/* Copyright 2009 Gary Briggs

This file is part of obdgpslogger.

obdgpslogger is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

obdgpslogger is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with obdgpslogger.  If not, see <http://www.gnu.org/licenses/>.
*/

/** \file
  \brief Functions to convert from OBD output to actual values
*/

// http://en.wikipedia.org/wiki/Table_of_OBD-II_Codes


float obdConvert_04    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A*100.0f/255.0f;
}


float obdConvert_05    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A-40.0f;
}


float obdConvert_06_09 (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return ((float)A-128.0f) * 100.0f/128.0f;
}


float obdConvert_0A    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A*3;
}


float obdConvert_0B    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A;
}


float obdConvert_0C    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (((float)A*256.0)+(float)B)/4.0f;
}


float obdConvert_0D    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A;
}


float obdConvert_0E    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A/2.0f-64.0f;
}


float obdConvert_0F    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A-40.0f;
}


float obdConvert_10    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (((float)A*256.0f)+(float)B)/100;
}


float obdConvert_11    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A*100.0f/255.0f;
}


float obdConvert_14_1B (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A*0.005f;
}


float obdConvert_1F    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A*256.0f+(float)B;
}


float obdConvert_21    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A*256.0f+(float)B;
}


float obdConvert_22    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return ((float)A*256.0f+(float)B)*0.079f;
}


float obdConvert_23    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return ((float)A*256.0f+(float)B)*10.0f;
}


float obdConvert_24_2B (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return ((float)A*256.0f+(float)B)*0.0000305f;
}


float obdConvert_2C    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A*100.0f/255.0f;
}


float obdConvert_2D    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A*0.78125f - 100.0f;
}


float obdConvert_2E    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A*100.0f/255.0f;
}


float obdConvert_2F    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A*100.0f/255.0f;
}


float obdConvert_30    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A;
}


float obdConvert_31    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A*256.0f+(float)B;
}


float obdConvert_32    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return ((float)A*256.0f+(float)B)/4.0f - 8192.0f;
}


float obdConvert_33    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A;
}


float obdConvert_34_3B (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return ((float)A*256.0f+(float)B)*0.0000305f;
}


float obdConvert_3C_3F (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return ((float)A*256.0f+(float)B)/10.0f-40.0f;
}


float obdConvert_42    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return ((float)A*256.0f+(float)B)/1000.0f;
}


float obdConvert_43    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return ((float)A*256.0f+(float)B)*100.0f/255.0f;
}


float obdConvert_44    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return ((float)A*256.0f+(float)B)*0.0000305f;
}


float obdConvert_45    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A*100.0f/255.0f;
}


float obdConvert_46    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A-40.0f;
}


float obdConvert_47_4B (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A*100.0f/255.0f;
}


float obdConvert_4C    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A*100.0f/255.0f;
}


float obdConvert_4D    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A*256.0f+(float)B;
}


float obdConvert_4E    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A*256.0f+(float)B;
}


float obdConvert_52    (unsigned int A, unsigned int B, unsigned int C, unsigned int D) {
	return (float)A*100.0f/255.0f;
}



