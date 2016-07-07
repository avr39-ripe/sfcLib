/*
 * wsbinconst.h
 *
 *  Created on: 6 июля 2016
 *      Author: shurik
 */

#ifndef LIB_WSBINCONST_WSBINCONST_H_
#define LIB_WSBINCONST_WSBINCONST_H_

namespace wsBinConst
{
//Frame header offsets
	const uint8_t wsCmd = 0; //Command type
	const uint8_t wsSysId = 1; //target sysId
	const uint8_t wsSubCmd = 2; //sub-command type
	const uint8_t wsPayLoadStart = 3;

	const uint8_t reservedCmd = 0;
	const uint8_t getCmd = 1;
	const uint8_t setCmd = 2;
	const uint8_t getResponse = 3;
	const uint8_t setAck = 4;
	const uint8_t setNack =5;

// sub-commands for App sysId=1
	const uint8_t scAppSetTime = 1;
	const uint8_t scAppGetStatus = 2;
// sub-commands for BinStateHttpClass sysId=2
	const uint8_t scBinStateGetName = 1;
	const uint8_t scBinStateGetState = 2;

};



#endif /* LIB_WSBINCONST_WSBINCONST_H_ */
