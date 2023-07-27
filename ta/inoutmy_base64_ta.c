#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <inoutmy_base64_ta.h>
#include <base64.h>

static TEE_Result base64_operation(uint32_t param_types, TEE_Param params[4], bool operation);
// 當 TA 的實例被創建時調用
TEE_Result TA_CreateEntryPoint(void)
{
	DMSG("has been called");

	return TEE_SUCCESS;
}

// 當打開一個新的Session時調用。在這個函數中，通常會對 TA 做一些全局初始化操作
TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
									TEE_Param __maybe_unused params[4],
									void __maybe_unused **sess_ctx)
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
											   TEE_PARAM_TYPE_NONE,
											   TEE_PARAM_TYPE_NONE,
											   TEE_PARAM_TYPE_NONE);

	DMSG("has been called");

	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Unused parameters */
	(void)&params;
	(void)&sess_ctx;

	/*
	 * The DMSG() macro is non-standard, TEE Internal API doesn't
	 * specify any means to logging from a TA.
	 */
	IMSG("TA initial finish!\n");

	/* If return value != TEE_SUCCESS the session will not be created. */
	return TEE_SUCCESS;
}


// 當 TA 被調用時調用
TEE_Result TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx,
									  uint32_t cmd_id,
									  uint32_t param_types, TEE_Param params[4])
{
	IMSG("TA_Invoke\n");
	(void)&sess_ctx; /* Unused parameter */

	switch (cmd_id)
	{
	case TA_INOUTMY_BASE64_CMD_ENCODE:
		return base64_operation(param_types, params,0);
	case TA_INOUTMY_BASE64_CMD_DECODE:
		return base64_operation(param_types, params,1);
	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}
}


static TEE_Result base64_operation(uint32_t param_types, TEE_Param params[4], bool operation)
{

	const void *inbuf;
	size_t inbuf_len;
	void *outbuf;
	size_t outbuf_len;
	bool operation_result;

	if (param_types != TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INOUT,
					TEE_PARAM_TYPE_VALUE_INOUT,
					TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE))
	{
		return TEE_ERROR_BAD_PARAMETERS;
	}
	IMSG("TA operation");

	inbuf = params[0].memref.buffer;
	inbuf_len = params[0].memref.size; 
	outbuf_len = _base64_enc_len(inbuf_len);
	outbuf = TEE_Malloc(outbuf_len,0);

	if (!outbuf)
    {
        return TEE_ERROR_OUT_OF_MEMORY;
    }
	IMSG("inbuf:%s inbuf_len:%lu outbuf:%s outbuf_len:%lu", (char*)inbuf,inbuf_len,(char*)outbuf,outbuf_len);


	if(!operation){
		IMSG("TA encode");
		operation_result = _base64_enc(inbuf,inbuf_len,outbuf,&outbuf_len);
	}
	else{
		IMSG("TA decode");
		operation_result = _base64_dec(inbuf,inbuf_len,outbuf,&outbuf_len);
	}
	 if (!operation_result)
    {
        TEE_Free(outbuf);
        return TEE_ERROR_GENERIC; 
    }
	IMSG("inbuf:%s inbuf_len:%lu outbuf:%s outbuf_len:%lu", (char*)inbuf,inbuf_len,(char*)outbuf,outbuf_len);

	 // 比較 inbuf_len 是否小於 outbuf_len
    if (inbuf_len < outbuf_len)
    {
		if(params[1].value.a != outbuf_len){ //代表沒回CA重新分配過
			params[1].value.a = outbuf_len;
			TEE_Free(outbuf); // 釋放無需複製的 outbuf
			IMSG("TA back CA to realloc");
			return TEE_ERROR_SHORT_BUFFER; // 返回錯誤碼，提示 CA 需要重新分配更大的緩衝區
		}
    }

	TEE_MemFill(params[0].memref.buffer,0,inbuf_len);
	TEE_MemMove(params[0].memref.buffer, outbuf, outbuf_len);
	params[0].memref.size = outbuf_len;


	IMSG("memref_buf:%s memref_buf_len:%u", (char*)params[0].memref.buffer,params[0].memref.size);

    TEE_Free(outbuf);

	return TEE_SUCCESS;
}


// 當關閉Session時調用
void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx)
{
	(void)&sess_ctx; /* Unused parameter */
	IMSG("Goodbye!\n");
}
void TA_DestroyEntryPoint(void)
{
	DMSG("has been called");
}