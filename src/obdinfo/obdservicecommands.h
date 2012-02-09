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
 \brief list of OBD service commands
 */
#ifndef __OBDSERVICECOMMANDS_H
#define __OBDSERVICECOMMANDS_H

#include <stdlib.h>
#include "obdconvertfunctions.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// structure to hold OBD service commands
struct obdservicecmd {
	unsigned int cmdid; ///< Command ID [eg 0C == engine rpm]
	int bytes_returned; ///< Number of bytes we expect back for this item. Elm327 optimisation [0 == no optimisation]
	const char *db_column; ///< Database column name.
	const char *human_name; ///< Human friendly name. http://www.kbmsystems.net/obd_tech.htm
	float min_value; ///< Minimum allowable value [after conversion]
	float max_value; ///< Maximum allowable value [after conversion]
	const char *units; ///< Human friendly representation of units
	OBDConvFunc conv; ///< Function to convert OBD values for this command to a useful number
	OBDConvRevFunc convrev; ///< Function to convert a useful number back to OBD values
};

/// Return the obdservicecmd struct for the requested db_column name
/** This function is O(n) including a strcmp. You probably don't
want to call it very often. */
struct obdservicecmd *obdGetCmdForColumn(const char *db_column);

/// Return the obdservicecmd struct for the requested db_column name
/** This function is O(n). You probably don't
want to call it very often. */
struct obdservicecmd *obdGetCmdForPID(const unsigned int pid);


// Fix "variable obdcmds defined but not used" warnings
#ifdef __GNUC__
#define VARIABLE_IS_NOT_USED __attribute__ ((unused))
#else
#define VARIABLE_IS_NOT_USED
#endif

/// List of all OBD Service commands
/// Borrowed from various sources, mainly http://en.wikipedia.org/wiki/Table_of_OBD-II_Codes
static struct obdservicecmd VARIABLE_IS_NOT_USED obdcmds_mode1[] = {
	{ 0x00, 4, NULL,            "PIDs supported 00-20" , 0, 0, "Bit Encoded", NULL, NULL },
	{ 0x01, 4, "dtc_cnt",            "Monitor status since DTCs cleared" , 0, 0, "Bit Encoded", NULL, NULL },
	{ 0x02, 4, "dtcfrzf",       "DTC that caused required freeze frame data storage" , 0, 0, "Bit Encoded", NULL, NULL },
	{ 0x03, 8, "fuelsys",       "Fuel system 1 and 2 status" , 0, 0, "Bit Encoded", NULL, NULL },
	{ 0x04, 2, "load_pct",      "Calculated LOAD Value" , 0, 100, "%", obdConvert_04, obdRevConvert_04 },
	{ 0x05, 1, "temp",          "Engine Coolant Temperature" , -40, 215, "Celsius", obdConvert_05, obdRevConvert_05 }, // J1979 calls this "ect"
	{ 0x06, 1, "shrtft13",      "Short Term Fuel Trim - Bank 1,3", -100, 99.22, "%", obdConvert_06_09, obdRevConvert_06_09 },
	{ 0x07, 1, "longft13",      "Long Term Fuel Trim - Bank 1,3", -100, 99.22, "%", obdConvert_06_09, obdRevConvert_06_09 },
	{ 0x08, 1, "shrtft24",      "Short Term Fuel Trim - Bank 2,4", -100, 99.22, "%", obdConvert_06_09, obdRevConvert_06_09 },
	{ 0x09, 1, "longft24",      "Long Term Fuel Trim - Bank 2,4", -100, 99.22, "%", obdConvert_06_09, obdRevConvert_06_09 },
	{ 0x0A, 1, "frp",           "Fuel Rail Pressure (gauge)", -100, 99.22, "%", obdConvert_0A, obdRevConvert_0A },
	{ 0x0B, 1, "map",           "Intake Manifold Absolute Pressure", 0, 765, "kPa", obdConvert_0B, obdRevConvert_0B },
	{ 0x0C, 2, "rpm",           "Engine RPM", 0, 16383.75, "rev/min", obdConvert_0C, obdRevConvert_0C },
	{ 0x0D, 1, "vss",           "Vehicle Speed Sensor", 0, 255, "km/h", obdConvert_0D, obdRevConvert_0D },
	{ 0x0E, 1, "sparkadv",      "Ignition Timing Advance for #1 Cylinder", -64, 63.5, "degrees relative to #1 cylinder", obdConvert_0E, obdRevConvert_0E },
	{ 0x0F, 1, "iat",           "Intake Air Temperature", -40, 215, "Celsius", obdConvert_0F, obdRevConvert_0F },
	{ 0x10, 2, "maf",           "Air Flow Rate from Mass Air Flow Sensor", 0, 655.35, "g/s", obdConvert_10, obdRevConvert_10 },
	{ 0x11, 1, "throttlepos",   "Absolute Throttle Position", 1, 100, "%", obdConvert_11, obdRevConvert_11 },
	{ 0x12, 1, "air_stat",      "Commanded Secondary Air Status" , 0, 0, "Bit Encoded", NULL, NULL },
	{ 0x13, 1, "o2sloc",        "Location of Oxygen Sensors" , 0, 0, "Bit Encoded", NULL, NULL },
	{ 0x14, 2, "o2s11",         "Bank 1 - Sensor 1/Bank 1 - Sensor 1 Oxygen Sensor Output Voltage / Short Term Fuel Trim", 0, 1.275, "V", obdConvert_14_1B, obdRevConvert_14_1B },
	{ 0x15, 2, "o2s12",         "Bank 1 - Sensor 2/Bank 1 - Sensor 2 Oxygen Sensor Output Voltage / Short Term Fuel Trim", 0, 1.275, "V", obdConvert_14_1B, obdRevConvert_14_1B },
	{ 0x16, 2, "o2s13",         "Bank 1 - Sensor 3/Bank 2 - Sensor 1 Oxygen Sensor Output Voltage / Short Term Fuel Trim", 0, 1.275, "V", obdConvert_14_1B, obdRevConvert_14_1B },
	{ 0x17, 2, "o2s14",         "Bank 1 - Sensor 4/Bank 2 - Sensor 2 Oxygen Sensor Output Voltage / Short Term Fuel Trim", 0, 1.275, "V", obdConvert_14_1B, obdRevConvert_14_1B },
	{ 0x18, 2, "o2s21",         "Bank 2 - Sensor 1/Bank 3 - Sensor 1 Oxygen Sensor Output Voltage / Short Term Fuel Trim", 0, 1.275, "V", obdConvert_14_1B, obdRevConvert_14_1B },
	{ 0x19, 2, "o2s22",         "Bank 2 - Sensor 2/Bank 3 - Sensor 2 Oxygen Sensor Output Voltage / Short Term Fuel Trim", 0, 1.275, "V", obdConvert_14_1B, obdRevConvert_14_1B },
	{ 0x1A, 2, "o2s23",         "Bank 2 - Sensor 3/Bank 4 - Sensor 1 Oxygen Sensor Output Voltage / Short Term Fuel Trim", 0, 1.275, "V", obdConvert_14_1B, obdRevConvert_14_1B },
	{ 0x1B, 2, "o2s24",         "Bank 2 - Sensor 4/Bank 4 - Sensor 2 Oxygen Sensor Output Voltage / Short Term Fuel Trim", 0, 1.275, "V", obdConvert_14_1B, obdRevConvert_14_1B },
	{ 0x1C, 1, "obdsup",        "OBD requirements to which vehicle is designed" , 0, 0, "Bit Encoded", NULL, NULL },
	{ 0x1D, 1, "o2sloc2",       "Location of oxygen sensors" , 0, 0, "Bit Encoded", NULL, NULL },
	{ 0x1E, 1, "pto_stat",      "Auxiliary Input Status" , 0, 0, "Bit Encoded", NULL, NULL },
	{ 0x1F, 2, "runtm",         "Time Since Engine Start", 0, 65535, "seconds", obdConvert_1F, obdRevConvert_1F },
	{ 0x20, 4, NULL,            "PIDs supported 21-40" , 0, 0, "Bit Encoded", NULL, NULL },
	{ 0x21, 4, "mil_dist",      "Distance Travelled While MIL is Activated", 0, 65535, "km", obdConvert_21, obdRevConvert_21 },
	{ 0x22, 2, "frpm",          "Fuel Rail Pressure relative to manifold vacuum", 0, 5177.265, "kPa", obdConvert_22, obdRevConvert_22 },
	{ 0x23, 2, "frpd",          "Fuel Rail Pressure (diesel)", 0, 655350, "kPa", obdConvert_23, obdRevConvert_23 },
	{ 0x24, 4, "lambda11",      "Bank 1 - Sensor 1/Bank 1 - Sensor 1 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage", 0, 2, "(ratio)", obdConvert_24_2B, obdRevConvert_24_2B },
	{ 0x25, 4, "lambda12",      "Bank 1 - Sensor 2/Bank 1 - Sensor 2 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage", 0, 2, "(ratio)", obdConvert_24_2B, obdRevConvert_24_2B },
	{ 0x26, 4, "lambda13",      "Bank 1 - Sensor 3 /Bank 2 - Sensor 1(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage", 0, 2, "(ratio)", obdConvert_24_2B, obdRevConvert_24_2B },
	{ 0x27, 4, "lambda14",      "Bank 1 - Sensor 4 /Bank 2 - Sensor 2(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage", 0, 2, "(ratio)", obdConvert_24_2B, obdRevConvert_24_2B },
	{ 0x28, 4, "lambda21",      "Bank 2 - Sensor 1 /Bank 3 - Sensor 1(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage", 0, 2, "(ratio)", obdConvert_24_2B, obdRevConvert_24_2B },
	{ 0x29, 4, "lambda22",      "Bank 2 - Sensor 2 /Bank 3 - Sensor 2(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage", 0, 2, "(ratio)", obdConvert_24_2B, obdRevConvert_24_2B },
	{ 0x2A, 4, "lambda23",      "Bank 2 - Sensor 3 /Bank 4 - Sensor 1(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage", 0, 2, "(ratio)", obdConvert_24_2B, obdRevConvert_24_2B },
	{ 0x2B, 4, "lambda24",      "Bank 2 - Sensor 4 /Bank 4 - Sensor 2(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage", 0, 2, "(ratio)", obdConvert_24_2B, obdRevConvert_24_2B },
	{ 0x2C, 1, "egr_pct",       "Commanded EGR", 0, 100, "%", obdConvert_2C, obdRevConvert_2C },
	{ 0x2D, 1, "egr_err",       "EGR Error", -100, 99.22, "%", obdConvert_2D, obdRevConvert_2D },
	{ 0x2E, 1, "evap_pct",      "Commanded Evaporative Purge", 0, 100, "%", obdConvert_2E, obdRevConvert_2E },
	{ 0x2F, 1, "fli",           "Fuel Level Input", 0, 100, "%", obdConvert_2F, obdRevConvert_2F },
	{ 0x30, 1, "warm_ups",      "Number of warm-ups since diagnostic trouble codes cleared", 0, 255, "", obdConvert_30, obdRevConvert_30 },
	{ 0x31, 2, "clr_dist",      "Distance since diagnostic trouble codes cleared", 0, 65535, "km", obdConvert_31, obdRevConvert_31 },
	{ 0x32, 2, "evap_vp",       "Evap System Vapour Pressure", -8192, 8192, "Pa", obdConvert_32, obdRevConvert_32 },
	{ 0x33, 1, "baro",          "Barometric Pressure", 0, 255, "kPa", obdConvert_33, obdRevConvert_33 },
	{ 0x34, 4, "lambdac11",     "Bank 1 - Sensor 1/Bank 1 - Sensor 1 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current", 0, 2, "(ratio)", obdConvert_34_3B, obdRevConvert_34_3B },
	{ 0x35, 4, "lambdac12",     "Bank 1 - Sensor 2/Bank 1 - Sensor 2 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current", 0, 2, "(ratio)", obdConvert_34_3B, obdRevConvert_34_3B },
	{ 0x36, 4, "lambdac13",     "Bank 1 - Sensor 3/Bank 2 - Sensor 1 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current", 0, 2, "(ratio)", obdConvert_34_3B, obdRevConvert_34_3B },
	{ 0x37, 4, "lambdac14",     "Bank 1 - Sensor 4/Bank 2 - Sensor 2 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current", 0, 2, "(ratio)", obdConvert_34_3B, obdRevConvert_34_3B },
	{ 0x38, 4, "lambdac21",     "Bank 2 - Sensor 1/Bank 3 - Sensor 1 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current", 0, 2, "(ratio)", obdConvert_34_3B, obdRevConvert_34_3B },
	{ 0x39, 4, "lambdac22",     "Bank 2 - Sensor 2/Bank 3 - Sensor 2 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current", 0, 2, "(ratio)", obdConvert_34_3B, obdRevConvert_34_3B },
	{ 0x3A, 4, "lambdac23",     "Bank 2 - Sensor 3/Bank 4 - Sensor 1 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current", 0, 2, "(ratio)", obdConvert_34_3B, obdRevConvert_34_3B },
	{ 0x3B, 4, "lambdac24",     "Bank 2 - Sensor 4/Bank 4 - Sensor 2 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current", 0, 2, "(ratio)", obdConvert_34_3B, obdRevConvert_34_3B },
	{ 0x3C, 2, "catemp11",      "Catalyst Temperature Bank 1 /  Sensor 1", -40, 6513.5, "Celsius", obdConvert_3C_3F, obdRevConvert_3C_3F },
	{ 0x3D, 2, "catemp21",      "Catalyst Temperature Bank 2 /  Sensor 1", -40, 6513.5, "Celsius", obdConvert_3C_3F, obdRevConvert_3C_3F },
	{ 0x3E, 2, "catemp12",      "Catalyst Temperature Bank 1 /  Sensor 2", -40, 6513.5, "Celsius", obdConvert_3C_3F, obdRevConvert_3C_3F },
	{ 0x3F, 2, "catemp22",      "Catalyst Temperature Bank 2 /  Sensor 2", -40, 6513.5, "Celsius", obdConvert_3C_3F, obdRevConvert_3C_3F },
	{ 0x40, 4, NULL,            "PIDs supported 41-60" , 0, 0, "Bit Encoded", NULL, NULL },
	{ 0x41, 4, NULL,            "Monitor status this driving cycle" , 0, 0, "Bit Encoded", NULL, NULL },
	{ 0x42, 2, "vpwr",          "Control module voltage", 0, 65535, "V", obdConvert_42, obdRevConvert_42 },
	{ 0x43, 2, "load_abs",      "Absolute Load Value", 0, 25700, "%", obdConvert_43, obdRevConvert_43 },
	{ 0x44, 2, "lambda",        "Fuel/air Commanded Equivalence Ratio", 0, 2, "(ratio)", obdConvert_44, obdRevConvert_44 },
	{ 0x45, 1, "tp_r",          "Relative Throttle Position", 0, 100, "%", obdConvert_45, obdRevConvert_45 },
	{ 0x46, 1, "aat",           "Ambient air temperature", -40, 215, "Celsius", obdConvert_46, obdRevConvert_46 },
	{ 0x47, 1, "tp_b",          "Absolute Throttle Position B", 0, 100, "%", obdConvert_47_4B, obdRevConvert_47_4B },
	{ 0x48, 1, "tp_c",          "Absolute Throttle Position C", 0, 100, "%", obdConvert_47_4B, obdRevConvert_47_4B },
	{ 0x49, 1, "app_d",         "Accelerator Pedal Position D", 0, 100, "%", obdConvert_47_4B, obdRevConvert_47_4B },
	{ 0x4A, 1, "app_e",         "Accelerator Pedal Position E", 0, 100, "%", obdConvert_47_4B, obdRevConvert_47_4B },
	{ 0x4B, 1, "app_f",         "Accelerator Pedal Position F", 0, 100, "%", obdConvert_47_4B, obdRevConvert_47_4B },
	{ 0x4C, 1, "tac_pct",       "Commanded Throttle Actuator Control", 0, 100, "%", obdConvert_4C, obdRevConvert_4C },
	{ 0x4D, 2, "mil_time",      "Time run by the engine while MIL activated", 0, 65525, "minutes", obdConvert_4D, obdRevConvert_4D },
	{ 0x4E, 2, "clr_time",      "Time since diagnostic trouble codes cleared", 0, 65535, "minutes", obdConvert_4E, obdRevConvert_4E },
	{ 0x4F, 4, NULL,            "External Test Equipment Configuration #1" , 0, 0, "Bit Encoded", NULL, NULL },
	{ 0x50, 4, NULL,            "External Test Equipment Configuration #2" , 0, 0, "Bit Encoded", NULL, NULL },
	{ 0x51, 2, "fuel_type",     "Fuel Type", 0, 0, "Bit Encoded", NULL, NULL },
	{ 0x52, 2, "alch_pct",      "Ethanol fuel %", 0, 100, "%", obdConvert_52, obdRevConvert_52 },

/*
	{ 0x53, 2, "evap_vpa",      "Absolute Evap System Vapor Pressure", 0, 327.675, "kPa", NULL, NULL },
	{ 0x54, 2, "evap_vp",       "Evap System Vapor Pressure", -32768, 32767, "Pa", NULL, NULL },
	{ 0x55, 2, "stso2ft1",      "Short Term Secondary O2 Sensor Fuel Trim – Bank 1/3", -100, 99.22, "%", NULL, NULL },
	{ 0x56, 2, "lgso2ft1",      "Long Term Secondary O2 Sensor Fuel Trim – Bank 1/3", -100, 99.22, "%", NULL, NULL },
	{ 0x57, 2, "stso2ft2",      "Short Term Secondary O2 Sensor Fuel Trim – Bank 2/4", -100, 99.22, "%", NULL, NULL },
	{ 0x58, 2, "lgso2ft2",      "Long Term Secondary O2 Sensor Fuel Trim – Bank 2/4", -100, 99.22, "%", NULL, NULL },
	{ 0x59, 2, "frp",           "Fuel Rail Pressure (absolute)", 0, 655350, "kPa", NULL, NULL },
	{ 0x5A, 1, "app_r",         "Relative Accellerator Pedal Position", 0, 100, "%", NULL, NULL },
	{ 0x5B, 1, "bat_pwr",       "Hybrid Battery Pack Remaining Life", 0, 100, "%", NULL, NULL },
	{ 0x5C, 1, "eot",           "Engine Oil Temperature", -40, 215, "Celcius", NULL, NULL },
	{ 0x5D, 2, "fuel_timing",   "Fuel Injection Timing", -310, 301.992, "degrees", NULL, NULL },
	{ 0x5E, 2, "fuel_rate",     "Engine Fuel Rate", 0, 3276.75, "L/h", NULL, NULL },
	{ 0x5F, 1, "emis_sup",      "Emission requirements to which vehicle is desinged", 0, 0, "Bit Encoded", NULL, NULL },
	{ 0x60, 4, NULL,            "PIDs supported 61-80" , 0, 0, "Bit Encoded", NULL, NULL },
	{ 0x61, 1, "tq_dd",         "Driver's Demand Engine - Percent Torque" , -125, 130, "%", NULL, NULL },
	{ 0x62, 1, "tq_act",        "Actual Engine - Percent Torque" , -125, 130, "%", NULL, NULL },
	{ 0x63, 2, "tq_ref",        "Engine Reference Torque" , 0, 65535, "Nm", NULL, NULL },
	{ 0x64, 5, "tq_max1",       "Engine Percent Torque Data" , -125, 130, "%", NULL, NULL },
	{ 0x65, 2, NULL,            "Auxilary Inputs/Outputs" , 0, 0, "Bit Encoded", NULL, NULL },
	{ 0x66, 5, "mafa",          "Mass Air Flow Sensor" , 0, 2047.96875, "g/s", NULL, NULL },
	{ 0x67, 3, "ect1",          "Engine Coolant Temperature" , -40, 215, "Celcius", NULL, NULL },
	{ 0x68, 7, "iat11",         "Intake Air Temperature Bank 1 Sensor 1" , -40, 215, "Celcius", NULL, NULL },
	{ 0x69, 6, NULL,            "Commanded EGR and EGR Error" , 0, 0, "Bit Encoded", NULL, NULL },
*/
	{ 0x00, 0, NULL,            NULL, 0, 0, NULL, NULL, NULL }
};

// Convert these two bytes to an OBDII error DTC, re-entrant flavor
/* \param buf pointer to buffer to fill
 \param number of bytes allocated in buf
 \return number of bytes written to buf
 */
int obderrconvert_r(char *buf, int n, unsigned int A, unsigned int B);

/// Convert these two bytes to an OBDII error DTC
/** Imagine something that's thread-safe. This isn't it. */
const char *obderrconvert(unsigned int A, unsigned int B);

#ifdef __cplusplus
}
#endif // __cplusplus


#endif // __OBDSERVICECOMMANDS_H

