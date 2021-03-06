/*********************************************************************************
*  Copyright (c) 2010-2011, Elliott Cooper-Balis
*                             Paul Rosenfeld
*                             Bruce Jacob
*                             University of Maryland 
*                             dramninjas [at] gmail [dot] com
*  All rights reserved.
*  
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*  
*     * Redistributions of source code must retain the above copyright notice,
*        this list of conditions and the following disclaimer.
*  
*     * Redistributions in binary form must reproduce the above copyright notice,
*        this list of conditions and the following disclaimer in the documentation
*        and/or other materials provided with the distribution.
*  
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
*  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
*  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
*  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
*  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*********************************************************************************/

#ifndef INIREADER_H
#define INIREADER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map> 
#include "SystemConfiguration.h"



#define DEFINE_UINT_PARAM(name, paramtype) {#name, &name, IniReader::UINT, IniReader::paramtype, false}
#define DEFINE_STRING_PARAM(name, paramtype) {#name, &name, IniReader::STRING, IniReader::paramtype, false}
#define DEFINE_FLOAT_PARAM(name,paramtype) {#name, &name, IniReader::FLOAT, IniReader::paramtype, false}
#define DEFINE_BOOL_PARAM(name, paramtype) {#name, &name, IniReader::BOOL, IniReader::paramtype, false}
#define DEFINE_UINT64_PARAM(name, paramtype) {#name, &name, IniReader::UINT64, IniReader::paramtype, false}

namespace DRAMSim
{
	using namespace std;

	class IniReader
	{
	public:
		typedef enum {STRING, UINT, UINT64, FLOAT, BOOL} VarType;
		typedef enum {SYS_PARAM, DEV_PARAM} ParamType;
		typedef enum {SYS_INI,DEV_INI} IniType;

		typedef struct
		{
			string iniKey; //for example "tRCD"

			void *variablePtr;
			VarType variableType;
			ParamType parameterType;
			bool wasSet;
		} ConfigMap;

		typedef map<string, string> OverrideMap;
		typedef OverrideMap::const_iterator OverrideIterator;

		static void SetKey(string key, string value, size_t lineNumber = 0, IniType iniType = SYS_INI);
		static void OverrideKeys(const OverrideMap *map);
		static void ReadIniFile(string filename, IniType iniType = SYS_INI);
		static void InitEnumsFromStrings();
		static bool CheckIfAllSet();
		static void WriteValuesOut(std::ofstream &visDataOut);

	private:
		static void WriteParams(std::ofstream &visDataOut, ParamType t);
		static void Trim(string &str);
	};
}


#endif
