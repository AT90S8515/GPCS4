#pragma once

#include "GPCS4Common.h"


namespace pssl
{;


enum PsslInstructionEncoding : uint32_t
{
	INST_ENCODING_SOP1 = 0x017D,
	INST_ENCODING_SOP2 = 0x02,
	INST_ENCODING_SOPK = 0x0B,
	INST_ENCODING_SOPC = 0x017E,
	INST_ENCODING_SOPP = 0x017F,
	
	INST_ENCODING_VOP1 = 0x3F,
	INST_ENCODING_VOP2 = 0x00,
	INST_ENCODING_VOP3 = 0x34,
	INST_ENCODING_VOPC = 0x3E,

	INST_ENCODING_SMRD = 0x18,

	INST_ENCODING_MUBUF = 0x38,
	INST_ENCODING_MTBUF = 0x3A,
	INST_ENCODING_MIMG  = 0x3C,

	INST_ENCODING_DS = 0x36,

	INST_ENCODING_VINTRP = 0x32,
	
	INST_ENCODING_EXP = 0x3E,
};



struct PsslShaderInstruction
{
	uint32_t placeHolder;
};


}  // namespace pssl
