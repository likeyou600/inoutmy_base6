#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* For the UUID (found in the TA's h-file(s)) */
#include <inoutmy_base64_ta.h>

int main(int argc, char *argv[])
{
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_INOUTMY_BASE64_UUID;
	uint32_t err_origin;


	// 初始化TEE上下文
	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

	/*-----------------------------------------------------------*/

	// 打開與TA的Session
	res = TEEC_OpenSession(&ctx, &sess, &uuid,
						   TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			 res, err_origin);

	/*-----------------------------------------------------------*/

	// 執行與 TA 之間的操作
	memset(&op, 0, sizeof(op));

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT,
					 TEEC_VALUE_INOUT,
					 TEEC_NONE, TEEC_NONE);


	/*----------------------------*/
	int operation;

	if(!strcmp(argv[1], "--encode"))
		operation = 0;
	else{
		if(!strcmp(argv[1], "--decode"))
			operation = 1;
		else{
			// 關閉Session並釋放資源
			TEEC_CloseSession(&sess);
			TEEC_FinalizeContext(&ctx);
			return 0;
		}
	}
	char *inbuf = NULL; 
	printf("Input:\n");
	scanf("%ms",&inbuf);
	op.params[0].tmpref.buffer = inbuf;
	op.params[0].tmpref.size = strlen(inbuf);

   	if (operation == 0){
		printf("Invoking TA to encode\n");
		res = TEEC_InvokeCommand(&sess, TA_INOUTMY_BASE64_CMD_ENCODE, &op,&err_origin);
	}
	else {
		printf("Invoking TA to decode\n");
		res = TEEC_InvokeCommand(&sess, TA_INOUTMY_BASE64_CMD_DECODE, &op,&err_origin);
	}

	if (res != TEEC_SUCCESS)
		if (res != TEEC_ERROR_SHORT_BUFFER){
			errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",res, err_origin);
		}
		else{
			printf("CA: TA requires a larger output buffer.\n");

			// 獲取 TA 需要的新的輸出緩衝區大小
			size_t new_outbuf_len = op.params[1].value.a;

			// 重新分配更大的緩衝區
			inbuf = realloc(inbuf, new_outbuf_len);
			if (inbuf == NULL)
				// 處理記憶體重新分配失敗的情況
				errx(1, "Memory reallocation failed.");


			// 再次呼叫 TA 進行操作
			if (operation == 0)
			{
				printf("Invoking TA to encode (retry)\n");
				res = TEEC_InvokeCommand(&sess, TA_INOUTMY_BASE64_CMD_ENCODE, &op, &err_origin);
			}
			else
			{
				printf("Invoking TA to decode (retry)\n");
				res = TEEC_InvokeCommand(&sess, TA_INOUTMY_BASE64_CMD_DECODE, &op, &err_origin);
			}

			if (res != TEEC_SUCCESS){
				errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",res, err_origin);
			}

		}
		
	printf("TA return output:\n");
	printf("%s\n",(char*)op.params[0].tmpref.buffer);

	
	free(inbuf);


	/*-----------------------------------------------------------*/

	// 關閉Session並釋放資源
	TEEC_CloseSession(&sess);

	TEEC_FinalizeContext(&ctx);

	return 0;
}
